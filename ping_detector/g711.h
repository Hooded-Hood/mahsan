//
// Created by hadi on 12/26/23.
//

#ifndef PING_DETECTOR_G711_H
#define PING_DETECTOR_G711_H

#include<cstdint>

/*
* Description:
*  Encodes a 16-bit signed integer using the A-Law.
* Parameters:
*  number - the number who will be encoded
* Returns:
*  The encoded number
*/
int8_t ALaw_Encode(int16_t number);
/*
* Description:
*  Decodes an 8-bit unsigned integer using the A-Law.
* Parameters:
*  number - the number who will be decoded
* Returns:
*  The decoded number
*/
int16_t ALaw_Decode(int8_t number);
/*
* Description:
*  Encodes a 16-bit signed integer using the mu-Law.
* Parameters:
*  number - the number who will be encoded
* Returns:
*  The encoded number
*/
int8_t MuLaw_Encode(int16_t number);
/*
* Description:
*  Decodes an 8-bit unsigned integer using the mu-Law.
* Parameters:
*  number - the number who will be decoded
* Returns:
*  The decoded number
*/
int16_t MuLaw_Decode(int8_t number);

#endif //PING_DETECTOR_G711_H
