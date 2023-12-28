#include <iostream>
#include <cstdlib>
#include <utility>
#include <PcapLiveDevice.h>
#include <PcapFileDevice.h>
#include "PcapLiveDeviceList.h"
#include "SystemUtils.h"
#include <vector>
#include <IPv4Layer.h>
#include <UdpLayer.h>
#include <PayloadLayer.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include "g711.h"

/**
* a callback function for the blocking mode capture which is called each time a packet is captured
*/
/*
static bool onPacketArrivesBlockingMode(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie)
{
    pcpp::Packet parsedPacket(packet);
    std::cout << "h1 pings h3\n";
    // return false means we don't want to stop capturing after this callback
    return false;
}
*/

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

std::vector<int16_t> decodeG711A(const std::vector<int8_t>& input) {
    std::vector<int16_t> output;
    for (int8_t x : input) {
        output.push_back(ALaw_Decode(x));
    }
    return output;
}

std::vector<int8_t> extractG711FromRTP(pcpp::Packet& packet) {
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
    std::vector<int8_t> payloadData(payloadLayer->getData() + 12, payloadLayer->getData() + payloadLayer->getDataLen());
    std::cout << payloadData.size() << " ";
    return payloadData;
}
//VoIPCallsFINAL.pcapng
std::vector<int16_t> total;
void storeMedia(const std::vector<int16_t>& v) {
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
void playPcap(const std::string& fileName) {// reads packets from file
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
    while (reader->getNextPacket(rawPacket)) {
        // Parse the packet
        pcpp::Packet packet(&rawPacket);

        std::vector<int8_t> g711Data = extractG711FromRTP(packet);

        std::vector<int16_t> decodedData = decodeG711A(g711Data);

        storeMedia(decodedData);

    }

    // close reader
    reader->close();
    // free reader memory because it was created by pcpp::IFileReaderDevice::getReader()
    delete reader;
}

void playDoublePcap(const std::string& fileName, const std::string& fileName2) {// reads packets from file
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
    while (reader->getNextPacket(rawPacket)) {
        // Parse the packet
        pcpp::Packet packet(&rawPacket);

        std::vector<int8_t> g711Data = extractG711FromRTP(packet);

        std::vector<int16_t> decodedData = decodeG711A(g711Data);

        storeMedia(decodedData);

    }

    // close reader
    reader->close();
    // free reader memory because it was created by pcpp::IFileReaderDevice::getReader()
    delete reader;

    // use the IFileReaderDevice interface to automatically identify file type (pcap/pcap-ng)
    // and create an interface instance that both readers implement
    pcpp::IFileReaderDevice* reader2 = pcpp::IFileReaderDevice::getReader(fileName2);
    // verify that a reader interface was indeed created
    if (reader2 == NULL) {
        std::cerr << "Cannot determine reader for file type" << std::endl;
        return;
    }
    // open the reader for reading
    if (!reader2->open()) {
        std::cerr << "Cannot open " << fileName << " for reading" << std::endl;
        return;
    }

    // Iterate over all packets in the capture file
    pcpp::RawPacket rawPacket2;
    while (reader2->getNextPacket(rawPacket2)) {
        // Parse the packet
        pcpp::Packet packet(&rawPacket2);

        std::vector<int8_t> g711Data = extractG711FromRTP(packet);

        std::vector<int16_t> decodedData = decodeG711A(g711Data);

        storeMedia(decodedData);

    }

    // close reader
    reader2->close();
    // free reader memory because it was created by pcpp::IFileReaderDevice::getReader()
    delete reader2;
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

int main(int argc, char* argv[]){
    std::cout << "Enter the number:\n"
            << "1.read from pcap file\n"
            << "2.read from interface packets\n"
            << "3.play from pcap file\n"
            << "4.play from double pcap file\n";
    int command;
    std::cin >> command;
    if (command == 1) {
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
        playPcap(fileName);
        // Play the decoded audio using SDL (or your preferred audio library)
        playAudioSDL(total);
    }
    else if (command == 4) {
        std::cout << "Enter the name of your File\n";
        std::string fileName;
        std::cin >> fileName;
        playPcap(fileName);
        std::cout << "\nEnter the name of your File\n";
        std::string fileName2;
        std::cin >> fileName2;
        playPcap(fileName2);
        // Play the decoded audio using SDL (or your preferred audio library)
        playAudioSDL(total);
    }
    else {
        std::cout << "INVALID\n";
    }

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
