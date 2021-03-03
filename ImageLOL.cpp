/*Copyright (c) 2021 MCredstoner2004*/

#include <iostream>
#include <stb_image.h>
#include <stb_image_write.h>
#include <string>
#include <fstream>
#include <limits>
#include <cstdint>
#include <math.h>

using u64 = std::uint64_t;
using byte = unsigned char;

enum channels {
		r=0, g=1, b=2
};
//simple nearest-neighbor sampling, might add another sampling method later.
byte sample_image(byte* image, int width, int height, double x, double y, channels channel) {	
	return image[((size_t)(y*(double)(height-1))*(size_t)width+(size_t)(x*(double)(width-1)))*3+channel];
}
//writes a byte into the lowest bit_n of each byte in the image, and updates the counters passed, masking each new byte with bit_mask.
void write_bits(byte* image, u64& current_pos, byte& current_bit, byte bits, int bit_n, byte bit_mask) {
	for(byte i = 0; i < 8; i++) {
		if(current_bit == 0) {
			image[current_pos] &= bit_mask;
		}
		image[current_pos] |= ((bits >> 8-i-1)&1) << (bit_n-current_bit-1); 
		if(current_bit >= bit_n - 1) {
			current_bit = 0;
			current_pos++;
		}else{
			current_bit++;
		}
	}
}
//reads a bit from the lowest bit_n bits of each byte in the image, and updates the counters passed.
byte read_bit(byte* image, u64& current_pos, byte& current_bit, int bit_n) {
	byte ret = (image[current_pos] >> (bit_n-current_bit-1)) & 1;
	if(current_bit >= bit_n - 1){
		current_pos++;
		current_bit = 0;
	}else{
		current_bit++;
	}
		return ret;
}


int main(int argc, char** argv) {
	int arg_count = 0;
	int bit_n = 3;	
	bool embed = false;
	bool extract = false;
	bool bad_usage = false;
	std::string input_image;
	std::string input_file;
	std::string output_image;
	//extremely primitive way of reading the arguments passed
	for(int i = 1; i < argc; i++){
		if(argv[i][0] != '-'){
			arg_count++;
			switch(arg_count){
			case 1:
				input_image = std::string(argv[i]);
				break;
			case 2:
				input_file = std::string(argv[i]);
				output_image = input_file + ".png";
				break;
			}
		}else{
			if(argv[i][1] == 'b'){
				if((argv[i][2] - '0' > 0) && (argv[i][2] - '0' < 9)){
					bit_n = argv[i][2] - '0';
					std::cout << "will use " << bit_n << " bits" << std::endl;
				}else{
					bad_usage = true;
				}
			} else {
				bad_usage = true;
			} 
		}
	}
	if(bad_usage){
		arg_count = 0;
	}
	switch(arg_count) {
	case 1:
		extract = true;
		break;
	case 2:
		embed = true;
		break;
	default:
		std::cout << "usage: ImageLOL <input-image> <input-file> [-b<bitdepth>] (embeds a file)\n";
		std::cout << "       ImageLOL <input-image> (extracts a file)" << std::endl;
	}
	if (extract||embed) {
		byte bit_mask =( 0xFF >> bit_n) << bit_n;
		int i_width, i_height;
		//reads the image as RGB, ignores any transparency
		byte* input_img = stbi_load(input_image.c_str(), &i_width, &i_height, nullptr, STBI_rgb);
		if(input_img != nullptr) {
			if(embed) {
				std::ifstream file;
				file.open(input_file, std::ios::binary);
				if(file.is_open()){
					file.ignore(std::numeric_limits<std::streamsize>::max());
					std::streamsize f_size = file.gcount();
					file.clear();
					file.seekg(0, std::ios::beg);
					std::cout << "file size: " << f_size << std::endl;
					if(f_size < UINT64_MAX){

						u64 file_size = (u64)f_size;
						u64 name_size = (u64)input_file.size();
						std::cout << "name size: " << name_size << std::endl;
						/* the total_size that will be stored is 1 byte for the numbers of bits to use, 64 bits
						 * for the length of the name, the name of the file, another 64 bits for the size of the
						 * file, and the file itself.
						 */
						u64 total_size = 1 + sizeof(u64) + name_size + sizeof(u64) + file_size;
						double in_out_ratio  =  total_size / ((double)i_width*(double)i_height * 3 * bit_n) * 8.0;
						if (in_out_ratio < 1.0){
							in_out_ratio = 1.0;
						}
						u64 o_width, o_height;
						o_width = ceil((double)i_width * sqrt(in_out_ratio));
						o_height = ceil((double)i_height * sqrt(in_out_ratio));
						while((o_width * o_height * 3 * 8)/bit_n < total_size){
							o_width++;
							o_height++;
						}
						/*output size will be (close) to the minimum image with the same proportions as the original
						 *that can hold atleast total_size bytes, or the original size if it can already store enough
						 * bytes.
						 */
						std::cout << "ratio: " << in_out_ratio << std::endl;
						std::cout << "output size: " << o_width << "," << o_height << std::endl;
						
						byte* output_img = new byte [o_width*o_height*3];
						//initialize the image with an scaled version of the original one.
						for(u64 i = 0; i < o_height; i++){
							for(u64 j = 0; j < o_width; j++){
												output_img[(i*o_width+j)*3] = sample_image(input_img, i_width, i_height, ((double)j/o_width),((double)i/o_height), channels::r);
												output_img[(i*o_width+j)*3+1] = sample_image(input_img, i_width, i_height, ((double)j/o_width),((double)i/o_height), channels::g);
												output_img[(i*o_width+j)*3+2] = sample_image(input_img, i_width, i_height, ((double)j/o_width),((double)i/o_height), channels::b);
							}
						}
						u64 current_pos = 1;
						byte current_bit = 0;
						u64 temp_length = 0;
						byte temp_byte = 0;
						for(int i = 0; i < 8; i++){
							if(i < bit_n){
								temp_byte |= (1 << i);
							}
						}
						//the first byte in the image is set to have only the lowest bit_n bits set.
						output_img[0] = temp_byte;
						//next 8 bytes will hold name_size.
						for(int i = 0; i < sizeof(u64); i++){
							temp_length = name_size >> ((sizeof(u64)-i-1)*8);
							temp_byte = (byte)(temp_length & 0xff);							
							write_bits(output_img, current_pos, current_bit, temp_byte, bit_n, bit_mask);
						}
						//next name_size bytes will hold the passed name.
						for(u64 i = 0; i < name_size; i++){
							temp_byte = input_file[i];
							write_bits(output_img, current_pos, current_bit, temp_byte, bit_n, bit_mask);
						}
						//next 8 bytes will hold file_size.
						for(int i = 0; i < sizeof(u64); i++){
							temp_length = file_size >> ((sizeof(u64)-i-1)*8);
							temp_byte = (byte)(temp_length & 0xff);
							write_bits(output_img, current_pos, current_bit, temp_byte, bit_n, bit_mask);
						}
						//next file_size bytes will hold the file
						for(u64 i = 0; i < file_size; i++){
							temp_byte = file.get();
							write_bits(output_img, current_pos, current_bit, temp_byte, bit_n, bit_mask);
						}
						//always writes the image into a png.
						stbi_write_png(output_image.c_str(), (int)o_width, (int)o_height, STBI_rgb, output_img, (int)o_width*STBI_rgb);								
					}else {
						std::cout << "file is too big, sorry :c" << std::endl;
					}
					file.close();
				}else{
					std::cout << "couldn't open file to embed" << std::endl;
				}
			}else{
				byte first_zero = 0;
				bit_n = 0;
				byte temp_byte = input_img[0];
				//the first byte is checked to know how many bits are used to store data.
				while(temp_byte > 0){
				bit_n ++;
				temp_byte = temp_byte >> 1;
				if(((temp_byte & 1) == 0) && (first_zero == 0)){
					first_zero = bit_n;
				}
				}
				u64 name_size = 0;
				u64 file_size = 0;
				//extracting the file will not start if the first byte is all 0's, or if it has a 0 between two 1's.
				if(first_zero == bit_n && bit_n != 0){
					std::cout << "The image uses " << bit_n << " bits per byte" << std::endl;
					u64 current_pos = 1;
					byte current_bit = 0;
					//read 8 bytes as name_size
					for(int i = 0; i < sizeof(u64)*8 ; i++){
						name_size |= (read_bit(input_img, current_pos, current_bit, bit_n) << (sizeof(u64)*8-i-1));
					} 
					std::string output_file((size_t)name_size, '\0');
					//read name_size bytes for the name
					for(int i = 0; i < name_size; i++){
						temp_byte = 0;
						for(int bit = 0; bit < 8; bit++){
							temp_byte |= (read_bit(input_img, current_pos, current_bit, bit_n) << (8-bit-1));
						}
						output_file[i] = temp_byte;
					}
					std::ofstream file;
					file.open(output_file, std::ios::binary|std::ios::trunc);
					//will only continue if it can open the output file, will probably replace this behaviour in the future.
					if(file.is_open()) {
						//read 8 bytes as file_size.
						for(int i = 0; i < sizeof(u64)*8 ; i++){
							file_size |= (read_bit(input_img, current_pos, current_bit, bit_n) << (sizeof(u64)*8-i-1));
						}
						byte* out_file = new byte [file_size];
						//read file_size bytes as the stored file.
						for(int i = 0; i < file_size; i++){
							temp_byte = 0;
							for(int bit = 0; bit < 8; bit++){
								temp_byte |= (read_bit(input_img, current_pos, current_bit, bit_n) << (8-bit-1));
							}
							out_file[i] = temp_byte;
						}
						//write the file.
						file.write(reinterpret_cast<const char*>(out_file), file_size);
						file.close();
					}else{
						std::cout << "couldn't open output file" << std::endl;
					}
					std::cout << "filename is: " << output_file << std::endl;
				} else {
					std::cout << "The image does not contain valid file data" << std::endl;
				}
			}
			stbi_image_free(input_img);
		}else{
			std::cout << "openning image failed" << std::endl;
		}
	}
}	
