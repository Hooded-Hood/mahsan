#include <iostream>
#include <fstream>
#include <cstdlib>
#include <memory>
#include <utility>
#include <PcapLiveDevice.h>
#include <PcapFileDevice.h>
#include "PcapLiveDeviceList.h"
#include "SdpLayer.h"
#include "SystemUtils.h"
#include <vector>
#include <queue>
#include <IPv4Layer.h>
#include <UdpLayer.h>
#include <PayloadLayer.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include "g711.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <netinet/in.h>
#include <EthLayer.h>
#include <SipLayer.h>
#include <set>
#include <chrono>
#include <memory>
#include<boost/shared_ptr.hpp>

static bool onPacketArrivesBlockingMode(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie)
{/**
* a callback function for the blocking mode capture which is called each time a packet is captured
*/
    pcpp::Packet parsedPacket(packet);
    std::cout << "h1 pings h3\n";
    // return false means we don't want to stop capturing after this callback
    return false;
}

void playWavFile(const std::string& fileName) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer initialization failed: " << Mix_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    Mix_Music* music = Mix_LoadMUS(fileName.c_str());
    if (!music) {
        std::cerr << "Failed to load WAV file: " << Mix_GetError() << std::endl;
        Mix_CloseAudio();
        SDL_Quit();
        return;
    }

    Mix_PlayMusic(music, 0);

    // Wait for the music to finish playing
    while (Mix_PlayingMusic()) {
        SDL_Delay(100);
    }

    Mix_FreeMusic(music);
    Mix_CloseAudio();
    SDL_Quit();
}

std::vector<int16_t> decodeG711A(const std::vector<int8_t>& input) {
    std::vector<int16_t> output;
    int i = 0;
    for (int8_t x : input) {
        i++;
        if (i >= 12) // header meader
            output.push_back(ALaw_Decode(x));
    }
    return output;
}

std::vector<int16_t> decodeG711U(const std::vector<int8_t>& input) {
    std::vector<int16_t> output;
    int i = 0;
    for (int8_t x : input) {
        i++;
        if (i >= 12) // header meader
            output.push_back(MuLaw_Decode(x));
    }
    return output;
}

std::vector<int16_t> decodeG729(const std::vector<int8_t>& input) {
    std::vector<int16_t> output;
    for (int8_t x : input) {
        //output.push_back(G729_Decode(x));
    }
    return output;
}

std::vector<int16_t> decodeGData(std::vector<int8_t>& gData) {
    std::vector<int16_t> pcmSamples;

    uint8_t payloadType = gData[1] & 0x7F;

    if (payloadType == 8) {
        pcmSamples = decodeG711A(std::vector<int8_t>(gData.begin()+12, gData.end()));
        //std::cout << "G711A";
    }
    else if (payloadType == 0) {
        pcmSamples = decodeG711U(std::vector<int8_t>(gData.begin()+12, gData.end()));
        //std::cout << "G711U";
    }
    else if (payloadType == 9) {
        //std::cout << "G722";
    }
    else if (payloadType == 18) {
        //pcmSamples = decodeG729(gData);
        //std::cout << "G729";
    }
    else {
        //std::cout << "G?";
    }

    return pcmSamples;
}

std::vector<int8_t> extractGdataFromRTP(const pcpp::Packet& packet) {
    pcpp::UdpLayer* udpLayer = packet.getLayerOfType<pcpp::UdpLayer>();
    if (!udpLayer) {
        std::cerr << "Packet does not contain a UDP layer" << std::endl;
        return {};
    }

    pcpp::PayloadLayer* payloadLayer = packet.getLayerOfType<pcpp::PayloadLayer>();
    if (!payloadLayer) {
        std::cerr << "Packet does not contain a payload layer" << std::endl;
        return {};
    }

    std::vector<int8_t> payloadData(payloadLayer->getData(),
                                    payloadLayer->getData() + payloadLayer->getDataLen());
    //std::cout << payloadData.size() << " ";
    return payloadData;
}

std::string extractCallIdentifier(const pcpp::Packet& packet) {
    std::string src = "";
    std::string dst = "";
    // extract source and dest address
    auto srdAddress = packet.getLayerOfType<pcpp::EthLayer>()->getSourceMac();
    auto dstAddress = packet.getLayerOfType<pcpp::EthLayer>()->getDestMac();
    auto srcIP = packet.getLayerOfType<pcpp::IPv4Layer>()->getSrcIPAddress();
    auto dstIP = packet.getLayerOfType<pcpp::IPv4Layer>()->getDstIPAddress();
    auto srcIP4 = packet.getLayerOfType<pcpp::IPv4Layer>()->getSrcIPv4Address();
    auto dstIP4 = packet.getLayerOfType<pcpp::IPv4Layer>()->getDstIPv4Address();

    src = srdAddress.toString() + srcIP.toString() + srcIP4.toString();
    dst = dstAddress.toString() + dstIP.toString() + dstIP4.toString();

    if (src > dst) {
        swap(src, dst);
    }

    std::string pre = "CODE:";
    pcpp::SipLayer* sipLayer = packet.getLayerOfType<pcpp::SipLayer>();
    if (sipLayer) {
        pcpp::SipRequestLayer *sipRequestLayer = packet.getLayerOfType<pcpp::SipRequestLayer>();
        if (sipRequestLayer != nullptr) {
            pcpp::SipRequestLayer::SipMethod sipMethod = sipRequestLayer->getFirstLine()->getMethod();
            if (sipMethod == pcpp::SipRequestLayer::SipMethod::SipBYE) {
                pre = "RBYE:";
            }
            else if (sipMethod == pcpp::SipRequestLayer::SipMethod::SipINVITE) {
                pre = "RINV:";
            }
            else if (sipMethod == pcpp::SipRequestLayer::SipMethod::SipACK) {
                pre = "RACK:";
            }
        }
        pcpp::SipResponseLayer *sipResponseLayer = packet.getLayerOfType<pcpp::SipResponseLayer>();
        if (sipResponseLayer != nullptr) {
            pcpp::SipResponseLayer::SipResponseStatusCode sipResponseStatusCode = sipResponseLayer->getFirstLine()->getStatusCode();
            if (sipResponseStatusCode == pcpp::SipResponseLayer::SipResponseStatusCode::Sip200OK) {
                pre = "SOKX:";
            }
            else if (sipResponseStatusCode == pcpp::SipResponseLayer::SipResponseStatusCode::Sip100Trying) {
                pre = "RTRY:";
            }
            else if (sipResponseStatusCode == pcpp::SipResponseLayer::SipResponseStatusCode::Sip180Ringing) {
                pre = "RRNG:";
            }
        }
    }

    return pre + src + dst;
}

void readPcap(const std::string& fileName = "packets.pcap") { // reads packets from file
    // use the IFileReaderDevice interface to automatically identify file type (pcap/pcap-ng)
    // and create an interface instance that both readers implement
    pcpp::IFileReaderDevice* reader = pcpp::IFileReaderDevice::getReader(fileName);
    // verify that a reader interface was indeed created
    if (reader == NULL) {
        std::cerr << "Cannot determine reader for file type" << std::endl;
        return;
    }
    // open the reader for reading
    if (!reader->open()) {
        std::cerr << "Cannot open " << fileName << " for reading" << std::endl;
        return;
    }
    pcpp::RawPacket rawPacket;
    int packetCounter = 0;
    while (reader->getNextPacket(rawPacket)) {
        packetCounter++;
    }
    std:: cout << packetCounter;
    // close reader
    reader->close();
    // free reader memory because it was created by pcpp::IFileReaderDevice::getReader()
    delete reader;

}

void storeMedia(std::vector<int16_t>& total, const std::vector<int16_t>& v) {

    for (auto x:v) {
        total.push_back(x);
    }
}

void playAudioSDL(const std::vector<int16_t>& pcmSamples) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq = 8000;           // Sample rate
    spec.format = AUDIO_S16SYS;  // 16-bit signed, little-endian
    spec.channels = 1;           // Mono
    spec.samples = 2048;         // Buffer size
    spec.callback = nullptr;     // No callback, use blocking
    spec.userdata = nullptr;

    SDL_AudioDeviceID audioDevice = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
    if (audioDevice == 0) {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    SDL_PauseAudioDevice(audioDevice, 0); // Unpause the audio device
    // Play the audio samples
    if (SDL_QueueAudio(audioDevice, pcmSamples.data(), pcmSamples.size() * sizeof(int16_t)) < 0) {
        std::cerr << "Failed to queue audio: " << SDL_GetError() << std::endl;
    }

    SDL_Delay(static_cast<Uint32>(pcmSamples.size() * 1000 / spec.freq)); // Wait for playback to complete
    //SDL_Delay(125);

    SDL_CloseAudioDevice(audioDevice);
    SDL_Quit();
}

std::string mergeFiles(const std::string& fileName, const std::string& fileName2) {// reads packets from file
    // use the IFileReaderDevice interface to automatically identify file type (pcap/pcap-ng)
    // and create an interface instance that both readers implement
    pcpp::IFileReaderDevice* reader = pcpp::IFileReaderDevice::getReader(fileName);
    pcpp::IFileReaderDevice* reader2 = pcpp::IFileReaderDevice::getReader(fileName2);
    // verify that a reader interface was indeed created
    if (reader == NULL) {
        std::cerr << "Cannot determine reader for file type" << std::endl;
        return "";
    }
    if (reader2 == NULL) {
        std::cerr << "Cannot determine reader for file type" << std::endl;
        return "";
    }
    // open the reader for reading
    if (!reader->open()) {
        std::cerr << "Cannot open " << fileName << " for reading" << std::endl;
        return "";
    }
    if (!reader2->open()) {
        std::cerr << "Cannot open " << fileName << " for reading" << std::endl;
        return "";
    }

    // create a pcap file writer. Specify file name and link type of all packets that
    // will be written to it
    pcpp::PcapFileWriterDevice pcapWriter("merge.pcap", pcpp::LINKTYPE_ETHERNET);

    // try to open the file for writing
    if (!pcapWriter.open())
    {
        std::cerr << "Cannot open output.pcap for writing" << std::endl;
        return "";
    }

    // Iterate over all packets in the capture file
    pcpp::RawPacket rawPacket;
    pcpp::RawPacket rawPacket2;
    while (reader->getNextPacket(rawPacket) and reader2->getNextPacket(rawPacket2)) {
        // Parse the packet
        pcpp::Packet packet(&rawPacket);
        pcpp::Packet packet2(&rawPacket2);

        pcapWriter.writePacket(rawPacket);
        pcapWriter.writePacket(rawPacket2);
    }

    // close reader
    reader->close();
    reader2->close();
    // free reader memory because it was created by pcpp::IFileReaderDevice::getReader()
    delete reader;
    delete reader2;

    pcapWriter.close();

    return "merge.pcap";
}

std::vector<std::string> separateVoIPCalls(const std::string& fileName) {// reads packets from file
    // use the IFileReaderDevice interface to automatically identify file type (pcap/pcap-ng)
    // and create an interface instance that both readers implement
    pcpp::IFileReaderDevice* reader = pcpp::IFileReaderDevice::getReader(fileName);
    // verify that a reader interface was indeed created
    if (reader == NULL) {
        std::cerr << "Cannot determine reader for file type" << std::endl;
        return {};
    }
    // open the reader for reading
    if (!reader->open()) {
        std::cerr << "Cannot open " << fileName << " for reading" << std::endl;
        return {};
    }


    std::unordered_map<std::string, std::vector<pcpp::Packet>> callRtpPackets;

    pcpp::RawPacket rawPacket;
    while (reader->getNextPacket(rawPacket)) {
        pcpp::Packet parsedPacket(&rawPacket);

        // Extract relevant information from the packet (e.g., source/destination IP, port numbers, protocol)
        pcpp::IPv4Layer* ipLayer = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();
        pcpp::UdpLayer* udpLayer = parsedPacket.getLayerOfType<pcpp::UdpLayer>();

        std::string callIdentifier = extractCallIdentifier(parsedPacket);

        callRtpPackets[callIdentifier].push_back(parsedPacket);
    }

    reader->close();
    delete reader;

    std::vector<std::string> calls;
    // Process each call separately
    for (const auto& pair : callRtpPackets) {
        const std::string& callIdentifier = pair.first;
        const std::vector<pcpp::Packet>& callRtpPackets = pair.second;        // Perform VoIP call analysis on callRtpPackets
        // (e.g., extracting audio streams, calculating statistics)
        std::cout << "Call Identifier: " << callIdentifier << std::endl;
        std::cout << "Number of RTP Packets in Call: " << callRtpPackets.size() << std::endl;

        // create a pcap file writer. Specify file name and link type of all packets that
        // will be written to it
        pcpp::PcapFileWriterDevice pcapWriter(callIdentifier + ".pcap", pcpp::LINKTYPE_ETHERNET);

        // try to open the file for writing
        if (!pcapWriter.open())
        {
            std::cerr << "Cannot open the file for writing" << std::endl;
            return {};
        }

        for (const auto& packet:callRtpPackets) {
            pcapWriter.writePacket(*packet.getRawPacket());
        }

        pcapWriter.close();

        calls.push_back(callIdentifier + ".pcap");
    }

    return calls;
}

void playPcap(std::vector<int16_t>& total, const std::string& fileName) {// reads packets from file
    // use the IFileReaderDevice interface to automatically identify file type (pcap/pcap-ng)
    // and create an interface instance that both readers implement
    pcpp::IFileReaderDevice* reader = pcpp::IFileReaderDevice::getReader(fileName);
    // verify that a reader interface was indeed created
    if (reader == NULL) {
        std::cerr << "Cannot determine reader for file type" << std::endl;
        return;
    }
    // open the reader for reading
    if (!reader->open()) {
        std::cerr << "Cannot open " << fileName << " for reading" << std::endl;
        return;
    }

    // Iterate over all packets in the capture file
    pcpp::RawPacket rawPacket;
    // parse the raw packet into a parsed packet

    while (reader->getNextPacket(rawPacket)) {
        // Parse the packet
        pcpp::Packet packet(&rawPacket);

        std::vector<int8_t> gData = extractGdataFromRTP(packet);

        if (gData.empty()) {
            continue;
        }

        std::vector<int16_t> decodedData = decodeGData(gData);

        storeMedia(total, decodedData);

    }

    // close reader
    reader->close();
    // free reader memory because it was created by pcpp::IFileReaderDevice::getReader()
    delete reader;
}

void readInterface(const std::string& interfaceIPAddr = "10.0.0.3", int timeout = 10) {
    pcpp::PcapLiveDevice* dev = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByIp(interfaceIPAddr);
    if (dev == NULL) {
        std::cerr << "Cannot find interface with IPv4 address of '" << interfaceIPAddr << "'" << std::endl;
        return;
    }
    if (!dev->open()) {// open the device before start capturing/sending packets
        std::cerr << "Cannot open device" << std::endl;
        return;
    }
    std::cout << std::endl << "Starting capture with packet vector..." << std::endl;
    pcpp::RawPacketVector packetVec;    // create an empty packet vector object
    dev->startCapture(packetVec);    // start capturing packets. All packets will be added to the packet vector
    pcpp::multiPlatformSleep(timeout);    // sleep for 10 seconds in main thread, in the meantime packets are captured in the async thread
    dev->stopCapture();    // stop capturing packets
    int packetConter = 0;
    // go over the packet vector and feed all packets to the stats object
    for (pcpp::RawPacketVector::ConstVectorIterator iter = packetVec.begin(); iter != packetVec.end(); iter++) {
        packetConter++;
    }
    std::cout << packetConter;
}

#define NUMBER_OF_WORKERS 3
#define NUMBER_OF_MAX_CALLS 1500
std::mutex mtx[NUMBER_OF_WORKERS], packetsMutex[NUMBER_OF_MAX_CALLS];
std::queue<std::unique_ptr<pcpp::RawPacket>> callPacketsQueue[NUMBER_OF_MAX_CALLS];
std::deque<int> writersTasks[NUMBER_OF_WORKERS];
int callOwner[NUMBER_OF_MAX_CALLS];
std::atomic<int> activeWriters(NUMBER_OF_WORKERS), callInQueues(0);
bool done = false;
std::set<int> allCalls, G711aCalls, G711uCalls, G722Calls, G729Calls, otherCalls;

void packetSorter(const std::string& fileName) {
    int cnt = 0;
    std::unordered_map<std::string, int> callID, connectionNumber;
    int callCounter = 0;
    std::fill(callOwner, callOwner + NUMBER_OF_MAX_CALLS, -1);
    /*pcpp::PcapFileWriterDevice pcapWriterG("good.pcap", pcpp::LINKTYPE_ETHERNET);
    pcpp::PcapFileWriterDevice pcapWriterB("bad.pcap", pcpp::LINKTYPE_ETHERNET);
    pcpp::PcapFileWriterDevice pcapWriterO("out.pcap", pcpp::LINKTYPE_ETHERNET);
    pcpp::PcapFileWriterDevice pcapWriterU("udp.pcap", pcpp::LINKTYPE_ETHERNET);

    if (!pcapWriterG.open()) {
        std::cerr << "Cannot open the file for writing" << std::endl;
        return;
    }
    if (!pcapWriterB.open()) {
        std::cerr << "Cannot open the file for writing" << std::endl;
        return;
    }
    if (!pcapWriterO.open()) {
        std::cerr << "Cannot open the file for writing" << std::endl;
        return;
    }
    if (!pcapWriterU.open()) {
        std::cerr << "Cannot open the file for writing" << std::endl;
        return;
    }
	*/
    pcpp::IFileReaderDevice* reader = pcpp::IFileReaderDevice::getReader(fileName);
    if (reader == NULL) {
        std::cerr << "Cannot determine reader for file type" << std::endl;
        return;
    }
    if (!reader->open()) {
        std::cerr << "Cannot open " << fileName << " for reading" << std::endl;
        return;
    }

    while (true) {
        pcpp::RawPacket* rawPacket = new pcpp::RawPacket;
        if (!reader->getNextPacket(*rawPacket)) {
            break;
        }
        pcpp::Packet parsedPacket(rawPacket);

        pcpp::SipRequestLayer* sipReqLayer = parsedPacket.getLayerOfType<pcpp::SipRequestLayer>();
        /*if (sipReqLayer) {
            pcpp::SipRequestLayer::SipMethod sipMethod = sipReqLayer->getFirstLine()->getMethod();
                std::cout<<sipMethod<<'\n';
        }*/
		pcpp::SipLayer* sipLayer = parsedPacket.getLayerOfType<pcpp::SipLayer>();
        pcpp::UdpLayer* udpLayer = parsedPacket.getLayerOfType<pcpp::UdpLayer>();
        pcpp::PayloadLayer* payloadLayer = parsedPacket.getLayerOfType<pcpp::PayloadLayer>();


        std::string callIdentifier = extractCallIdentifier(parsedPacket);

        if (!udpLayer) {  // those idiots who wont be recognized in sip
//            49 4e 56 49 54 45
            std::string s(parsedPacket.getLastLayer()->getData() + 8, parsedPacket.getLastLayer()->getData() + 14);
            if (s == "INVITE") {
                callIdentifier[0] = 'R';
                callIdentifier[1] = 'I';
                callIdentifier[2] = 'N';
                callIdentifier[3] = 'V';
                //std::cout<<cnt++<<"\n";
            }
           // for (auto x:s)
              //  std::cout<<x;
            //std::cout << '\n';
            //pcapWriterU.writePacket(*rawPacket);
        }
        //pcpp::SipLayer* sipLayer = parsedPacket.getLayerOfType<pcpp::SipLayer>();

/*
        if (sipLayer) {
            if (callIdentifier[0] == 'R') {
                if (callIdentifier.substr(1,3) == "INV") {

                }
                else if (callIdentifier.substr(1,3) == "BYE") {
                    // NO NEED HERE
                }
                else if (callIdentifier.substr(1,3) == "ACK") {
                    // NO NEED HERE
                }
                else {
                    // NO NEED HERE
                    //pcapWriterB.writePacket(*rawPacket);
                }
            }
            else if (callIdentifier[0] == 'S') {
                if (callIdentifier.substr(1,3) == "OKX") {
                    // NO NEED HERE
                }
                else if (callIdentifier.substr(1,3) == "TRY") {
                    // NO NEED HERE
                }
                else if (callIdentifier.substr(1,3) == "RNG") {
                    // NO NEED HERE
                }
                else {
                    // NO NEED HERE
                }
            }
            else {
                //pcapWriterB.writePacket(*rawPacket);
            }
            continue;
        }

*/
        if (callIdentifier.substr(0,4) == "RINV") {
            callIdentifier[0] = 'C';
            callIdentifier[1] = 'O';
            callIdentifier[2] = 'D';
            callIdentifier[3] = 'E';
            if (!connectionNumber[callIdentifier]) {
                connectionNumber[callIdentifier] = 1;
            }
            else {
                connectionNumber[callIdentifier] += 1;
            }
            callCounter++;
            callID[callIdentifier + std::to_string(connectionNumber[callIdentifier])] = callCounter;

            //pcapWriterG.writePacket(*rawPacket);
            continue;
        }


        if (!payloadLayer) {
            //pcapWriterO.writePacket(*rawPacket);
            continue;
        }
        int len = payloadLayer->getPayloadLen() - 12;
        if (!(len == 160 or len == 80 or len == 20 or len == 10)) {
            continue;
        }
        callIdentifier += std::to_string(connectionNumber[callIdentifier]);

        int writerID = callID[callIdentifier] % NUMBER_OF_WORKERS;
        bool unowneredCallArrived = false;

        {
            // adding packet to its queue
            std::lock_guard<std::mutex> packetLock(packetsMutex[callID[callIdentifier]]);
            callPacketsQueue[callID[callIdentifier]].push(std::unique_ptr<pcpp::RawPacket>(rawPacket));
            // if it doesn't have an owner you must give it an owner
            if (callOwner[callID[callIdentifier]] == -1) {
                callOwner[callID[callIdentifier]] = writerID;
//            std::cout << callInQueues << " " << callOwner[callID[callIdentifier]] << " " << callID[callIdentifier] << "\n";
                unowneredCallArrived = true;
            }
        }


        if (unowneredCallArrived) {// if it doesn't have an owner give it to an owner
            callInQueues += 1;
            {
                std::lock_guard<std::mutex> writerLock(mtx[writerID]);
                //std::cout<<"lock in sorter\n";
                writersTasks[writerID].push_back(callID[callIdentifier]);
                //std::cout<<"unlock in sorter\n";
            }
        }
            //cv[id].notify_one();
        //std::cout<<"notify all\n";

    }
/*
    pcapWriterG.close();
    pcapWriterB.close();
    pcapWriterO.close();
    pcapWriterU.close();*/
    reader->close();
    delete reader;
    std::cout<<"END OF FILE. call counter: " << callCounter << '\n';
    done = 1;
//    for (int i = 0; i < NUMBER_OF_WORKERS; i++) {
//        cv[i].notify_one();
//    }
}

void callWriter(const int writerID) {
    int cnt = 0;
    while (true) {
        int currentCallID = -1;
        {
            // Check if there are tasks to do, and if not, try to steal from other writers
            std::lock_guard<std::mutex> currentWriterLock(mtx[writerID]);
            //cv.wait(currentWriterLock);
            if (!writersTasks[writerID].empty()) {
                currentCallID = writersTasks[writerID].front();
            }
        }

        if (currentCallID == -1) {

            for (int victim = 0; victim < NUMBER_OF_WORKERS; victim++) {
                if (victim == writerID) {
                    continue;
                }

                //std::unique_lock<std::mutex> victimLock(mtx[victim]);
                //if (victimLock.try_lock())
                {
                    std::lock_guard<std::mutex> victimLock(mtx[victim]);
                    if (writersTasks[victim].size() > 1) {
                        currentCallID = writersTasks[victim].back();
                        writersTasks[victim].pop_back();
                        break;
                    }
                    //victimLock.unlock();
                }

            }

            if (currentCallID != -1) {
                std::lock_guard<std::mutex> currentWriterLock(mtx[writerID]);
                writersTasks[writerID].push_front(currentCallID);
            }
        }
        {
            std::lock_guard<std::mutex> currentWriterLock(mtx[writerID]);
            if (!writersTasks[writerID].empty()) {
                currentCallID = writersTasks[writerID].front();
            }
        }

        if (currentCallID == -1) {
            if (callInQueues < activeWriters and done) {
                break;
            }
            continue; // No tasks to perform
        }
//        cv[id].wait(lock, [id] {
//
//            return ((!callPackets[id].empty()) or done); });
//        if (callInQueues <= 1 and done) {
//            lock.unlock();
//            //std::cout<< "unlock in worker" << id << "at last \n";
//            break;
//        }

        while (true) {
            // takeing a packet
            std::unique_lock<std::mutex> packetLock(packetsMutex[currentCallID]);
            //pcpp::Packet parsedPacket;
            //    std::lock_guard<std::mutex> packetLock(packetsMutex[currentCallID]);
            if (callPacketsQueue[currentCallID].empty()) {
                packetLock.unlock();
                break; // end of loop
            }
            pcpp::Packet parsedPacket(callPacketsQueue[currentCallID].front().release());
            callPacketsQueue[currentCallID].pop();
            packetLock.unlock();

            // extracting codec data from packets
            std::vector<int8_t> gData = extractGdataFromRTP(parsedPacket);

            // we don't have data go home toon
            if (gData.empty()) {
                continue;
            }

            // checking codec type
            uint8_t payloadType = gData[1] & 0x7F;
            int len = gData.size() - 12;

            allCalls.insert(currentCallID);
            if (payloadType == 8 and len == 160) {
                G711aCalls.insert(currentCallID);
            }
            else if (payloadType == 0 and (len == 160 or len == 80)) {
                G711uCalls.insert(currentCallID);
            }
            else if (payloadType == 9) {
                G722Calls.insert(currentCallID);
                continue;
            }
            else if (payloadType == 18 and (len == 20 or len == 10)) {
                G729Calls.insert(currentCallID);
                continue;
            }
            else {
                otherCalls.insert(currentCallID);
                continue;
            }

            // decode data
            std::vector<int16_t> pcmSamples = decodeGData(gData);

            // open file to write pcm data
            std::ofstream outputFile("call" + std::to_string(currentCallID) + ".pcm",
                                     std::ios::binary | std::ios::app);
            if (!outputFile.is_open()) {
                std::cerr << "cannot open call file";
                return;
            }
            outputFile.write(reinterpret_cast<char*>(&pcmSamples[0]), pcmSamples.size() * sizeof(short));
            outputFile.close();

        }

        // Release ownership and update the queue state
        {
            std::lock_guard<std::mutex> currentWriterLock(mtx[writerID]);
            //std::cout<<writersTasks[writerID].front() << " ";

            callOwner[currentCallID] = -1;
            writersTasks[writerID].pop_front();

        }
        callInQueues--;
        if (callInQueues < 1 and done) {
            break;
        }
    }
}

std::queue<std::pair<std::unique_ptr<int>, std::unique_ptr<pcpp::RawPacket>>> callPackets[NUMBER_OF_WORKERS];
std::condition_variable cv[NUMBER_OF_WORKERS];

void packetSorter2(const std::string& fileName) {

    std::unordered_map<std::string, int> callID, connectionNumber;
    int callCounter = 0;

    pcpp::IFileReaderDevice* reader = pcpp::IFileReaderDevice::getReader(fileName);
    if (reader == NULL) {
        std::cerr << "Cannot determine reader for file type" << std::endl;
        return;
    }
    if (!reader->open()) {
        std::cerr << "Cannot open " << fileName << " for reading" << std::endl;
        return;
    }

    while (true) {
        pcpp::RawPacket* rawPacket = new pcpp::RawPacket;
        if (!reader->getNextPacket(*rawPacket)) {
            break;
        }
        pcpp::Packet parsedPacket(rawPacket);
        pcpp::UdpLayer* udpLayer = parsedPacket.getLayerOfType<pcpp::UdpLayer>();
        if (!udpLayer) {
            continue;
        }
        pcpp::PayloadLayer* payloadLayer = parsedPacket.getLayerOfType<pcpp::PayloadLayer>();
        pcpp::SipLayer* sipLayer = parsedPacket.getLayerOfType<pcpp::SipLayer>();
        if (!payloadLayer and !sipLayer) {
            continue;
        }
        std::string callIdentifier = extractCallIdentifier(parsedPacket);

        if (sipLayer) {
            if (callIdentifier[0] == 'R') {
                if (callIdentifier.substr(1,3) == "INV") {
                    callIdentifier[0] = 'C';
                    callIdentifier[1] = 'O';
                    callIdentifier[2] = 'D';
                    callIdentifier[3] = 'E';
                    if (!connectionNumber[callIdentifier]) {
                        connectionNumber[callIdentifier] = 1;
                    }
                    else {
                        connectionNumber[callIdentifier] += 1;
                    }
                    callCounter++;
                    callID[callIdentifier + std::to_string(connectionNumber[callIdentifier])] = callCounter;
                }
                else if (callIdentifier.substr(1,3) == "BYE") {
                    // NO NEED HERE
                }
                else if (callIdentifier.substr(1,3) == "ACK") {
                    // NO NEED HERE
                }
                else {
                    // NO NEED HERE
                }
            }
            else if (callIdentifier[0] == 'S') {
                if (callIdentifier.substr(1,3) == "OKX") {
                    // NO NEED HERE
                }
                else if (callIdentifier.substr(1,3) == "TRY") {
                    // NO NEED HERE
                }
                else if (callIdentifier.substr(1,3) == "RNG") {
                    // NO NEED HERE
                }
                else {
                    // NO NEED HERE
                }
            }
            continue;
        }

        callIdentifier += std::to_string(connectionNumber[callIdentifier]);

        int id = callCounter % NUMBER_OF_WORKERS;
        {std::lock_guard<std::mutex> lock(mtx[id]);
            //std::cout<<"lock in sorter\n";
            callPackets[id].push({std::unique_ptr<int>(new int(callCounter)), std::unique_ptr<pcpp::RawPacket>(rawPacket)});
            //std::cout<<"unlock in sorter\n";
        }cv[id].notify_all();
        //std::cout<<"notify all\n";
    }

    reader->close();
    delete reader;
    std::cout<<"END OF FILE. call counter: " << callCounter << '\n';
    done = 1;

}

void callWriter2(const int id) {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx[id]);
        //std::cout <<"going to lock" << id << '\n';
        cv[id].wait(lock, [id] { return ((!callPackets[id].empty()) or done); });
        //std::cout<<"lock in worker" << id << '\n';
        if (done and callPackets[id].empty()) {
            lock.unlock();
          //  std::cout << "unlock in worker" << id << "at last \n";
            break;
        }

        int callID = *callPackets[id].front().first;
        pcpp::Packet parsedPacket(callPackets[id].front().second.release());
        callPackets[id].pop();
        lock.unlock();
        //std::cout<<"unlock in worker" << id << '\n';

        std::vector<int8_t> gData = extractGdataFromRTP(parsedPacket);

        if (gData.empty()) {
            continue;
        }

        uint8_t payloadType = gData[1] & 0x7F;

        allCalls.insert(callID);
        if (payloadType == 8) {
            G711aCalls.insert(callID);
        } else if (payloadType == 0) {
            G711uCalls.insert(callID);
        } else if (payloadType == 9) {
            G722Calls.insert(callID);
            continue;
        } else if (payloadType == 18) {
            G729Calls.insert(callID);
            continue;
        } else {
            continue;
        }

        std::vector<int16_t> pcmSamples = decodeGData(gData);

//            std::ofstream outputFile("call" + std::to_string(callID[callIdentifier]) + ".wav", std::ios::binary | std::ios::app);
        std::ofstream outputFile("call" + std::to_string(callID) + ".pcm",
                                 std::ios::binary | std::ios::app);

        if (!outputFile.is_open()) {
            std::cerr << "cannot open call file";
            return;
        }

        outputFile.write(reinterpret_cast<char *>(&pcmSamples[0]), pcmSamples.size() * sizeof(short));
        outputFile.close();
    }
}

int main(int argc, char* argv[]){
    auto start = std::chrono::high_resolution_clock::now();
    int command = 5; /*
    std::cout << "Enter the number:\n"
              << "0.The REAL thing, ping detector\n"
              << "1.read from pcap file\n"
              << "2.read from interface packets\n"
              << "3.play from pcap file\n"
              << "4.play from double pcap file\n"
              << "5.separate calls in multi-thread\n"
              << "6.separate calls in multi-thread (primary version)\n";
    std::cin >> command;*/
    if (command == 0) {
        /*
        dev->startCaptureBlockingMode(onPacketArrivesBlockingMode, NULL, 0);
        pcpp::IPFilter srcipFilter("10.0.0.1", pcpp::SRC); // Capture only packets with the specified source IP
        pcpp::IPFilter dstipFilter("10.0.0.3", pcpp::DST); // Capture only packets with the specified destination IP
        pcpp::AndFilter ipFilter;
        ipFilter.addFilter(&srcipFilter);
        ipFilter.addFilter(&dstipFilter);
        // create a filter instance to capture only ICMP traffic
        pcpp::ProtoFilter protocolFilter(pcpp::ICMP);

        // create an AND filter to combine both filters - capture only TCP traffic on port 80
        pcpp::AndFilter andFilter;
        andFilter.addFilter(&ipFilter);
        andFilter.addFilter(&protocolFilter);

        // set the filter on the device
        dev->setFilter(andFilter);
        */
    }
    else if (command == 1) {
        std::cout << "Enter the name of your File\n";
        std::string fileName;
        std::cin >> fileName;
        readPcap(fileName);
    }
    else if (command == 2) {
        std::cout << "Enter the ip of your interface\n";
        std::string interfaceIp;
        std::cin >> interfaceIp;
        std::cout << "Enter the timeout for your capture\n";
        int timeout;
        std::cin >> timeout;
        readInterface(interfaceIp, timeout);
    }
    else if (command == 3) {
        std::cout << "Enter the name of your File\n";
        std::string fileName;
        std::cin >> fileName;
        std::vector<int16_t> total;
        playPcap(total, fileName);
        playAudioSDL(total);
    }
    else if (command == 4) {
        std::vector<int16_t> total;
        std::cout << "Enter the name of your File\n";
        std::string fileName;
        std::cin >> fileName;
        std::cout << "Enter the name of your File\n";
        std::string fileName2;
        std::cin >> fileName2;
        playPcap(total, mergeFiles(fileName, fileName2));
        // Play the decoded audio using SDL (or your preferred audio library)
        playAudioSDL(total);
    }
    else if (command == 5) {
        //        std::cout << "Enter the name of your File\n";
        std::string fileName;
        //        std::cin >> fileName;
        fileName = "large.pcap";
        std::thread sorterThread(packetSorter, fileName);
        std::thread writerThreads[NUMBER_OF_WORKERS];
        for (int i = 0; i < NUMBER_OF_WORKERS; i++) {
            writerThreads[i] = std::thread(callWriter, i);
        }
        sorterThread.join();
        for (int i = 0; i < NUMBER_OF_WORKERS; i++) {
            writerThreads[i].join();
            activeWriters -= 1;
        }
        std::cout << "allCalls: " << allCalls.size()
                  << " G711aCalls: " << G711aCalls.size()
                  << " G711uCalls: " << G711uCalls.size()
                  << " G722Calls: " << G722Calls.size()
                  << " G729Calls: " << G729Calls.size() << '\n';
    }
    else if (command == 6) {
        //        std::cout << "Enter the name of your File\n";
        std::string fileName;
        //        std::cin >> fileName;
        fileName = "large.pcap";
        std::thread sorterThread(packetSorter2, fileName);
        std::thread writerThreads[NUMBER_OF_WORKERS];
        for (int i = 0; i < NUMBER_OF_WORKERS; i++) {
            writerThreads[i] = std::thread(callWriter2, i);
        }
        sorterThread.join();
        for (int i = 0; i < NUMBER_OF_WORKERS; i++) {
            writerThreads[i].join();
        }
        std::cout << "allCalls: " << allCalls.size()
                  << " G711aCalls: " << G711aCalls.size()
                  << " G711uCalls: " << G711uCalls.size()
                  << " G722Calls: " << G722Calls.size()
                  << " G729Calls: " << G729Calls.size() << '\n';
    }
    else {
        std::cout << "INVALID\n";
    }

    auto stop = std::chrono::high_resolution_clock::now();

    // Calculate the duration in milliseconds
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

    // Output the duration
    std::cout << "Time taken by function: " << duration.count() << " milliseconds" << std::endl;
}
