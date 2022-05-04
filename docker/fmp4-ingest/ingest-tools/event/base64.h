//
//  base64 encoding and decoding with C++.
//  Version: 1.01.00
//

#ifndef BASE64_H_C0CE2A47_D10E_42C9_A27C_C883944E704A
#define BASE64_H_C0CE2A47_D10E_42C9_A27C_C883944E704A

#include <string>
#include <vector>

std::string base64_encode(const unsigned char* bytes_to_encode, std::size_t in_len);

std::vector<uint8_t> base64_decode(const std::string& encoded_string);
#endif /* BASE64_H_C0CE2A47_D10E_42C9_A27C_C883944E704A */