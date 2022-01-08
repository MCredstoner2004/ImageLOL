/*Copyright (c) 2021 MCredstoner2004*/

#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <system_error>
#include <limits>
#include <memory>
#include <cstdint>
#include <cerrno>

#include <png.h>
#include <argh.h>
#include <stb_image.h>

#include <ImageLOL.h>

using u64 = ImageLOL::u64;
using byte = ImageLOL::byte;
using path = std::filesystem::path;

enum channels
{
		r=0, g=1, b=2, a=3
};
//simple nearest-neighbor sampling, might add another sampling method later.
byte sample_image(byte* image, int width, int height, double x, double y, int num_channels, channels channel)
{
	return image[(static_cast<size_t>(y * static_cast<double>(height-1)) * static_cast<size_t>(width) + static_cast<size_t>(x * static_cast<double>(width-1))) * num_channels + channel];
}


void show_usage(bool show_help = false) {
	const std::string help_string = "ImageLOL is a small program to store and extract files from the least significant bits of images (https://github.com/MCredstoner2004/ImageLOL)";
	const std::string usage_string = R"(Usage:
to embed a file:
  imagelol <input-image> <input-file> [options]
to extract a file:
  imagelol <input-image> [options]
Options:
  --help                 Show help and usage, then quit
  -b, --bitdepth <1-8>   Use the <bitdepth> lowest bits, only while embedding, defaults to 3
  -o, --output <file>    Write to the specified file, defaults to:
                           <input-file>.png for embedding
                           the stored file name for extraction
  -w, --overwrite        Overwrite the file in case it exists, instead of quitting)";
	if (show_help)
	{
		std::cout << help_string << "\n" << usage_string  << std::endl;
	} else
	{
		std::cerr << usage_string << std::endl;
	}

}

int main(int argc, char** argv)
{

	int bit_depth = 3; // the bit depth to be use for embedding, 3 is the default
	bool embed = false; // wheter to embed or not, only one of this and extract will be set at a time
	bool extract = false; // wheter to extract or not, only one of this and embed will be set at a time
	bool output_filename_set = false; // wheter the output option was used or not
	bool overwrite = false;
	argh::parser cmdl = argh::parser({"-o", "--output", "-b", "--bit-depth"}); // TODO: implement overwrite
	cmdl.parse(argc,argv);
	path input_image;
	path input_file;
	path output_filename;
	int result = 1;

	if (cmdl[{"-h", "--help"}])
	{
		show_usage(true);
	}
	else
	{
		switch (cmdl.size()) {
		case 2:
				extract = true;
				input_image = path(cmdl[1]);
				break;
		case 3:
				embed = true;
				input_image = path(cmdl[1]);
				input_file = path(cmdl[2]);
				break;
		default:
				show_usage();
		}
	}
	output_filename_set = static_cast<bool>(cmdl({"-o", "--output"}));
	if (output_filename_set)
	{
		output_filename = path(cmdl({"-o", "--output"}).str());
	}

	if (static_cast<bool>(cmdl({"-b", "--bit-depth"})))
	{
		if (extract)
		{
			std::clog << "bit-depth does nothing when extracting, ignoring" << std::endl;
		}
		if (embed && (!(cmdl({"-b", "--bit-depth"}) >> bit_depth) || (bit_depth < 1 || bit_depth > 8 )))
		{
			std::cerr << "bit depth must be an integer from 1 to 8" << std::endl;
			embed = false;
			show_usage();
		}
	}

	overwrite = cmdl[{"--overwrite", "-w"}];

	if (extract||embed)
	{
		bool is_png;
		bool could_read_img = false;
		int num_channels = 4;
		size_t input_bytes;
		std::vector<byte> image_bytes;
		int i_width = 0, i_height = 0;
		//reads the image as RGB, ignores any transparency
		std::ifstream image_file;
		image_file.open(input_image, std::ios::binary);
		if (image_file.is_open())
		{
			image_file.ignore(std::numeric_limits<std::streamsize>::max()); // seek until EOF
			std::streamsize image_file_size = image_file.gcount(); // get total number of bytes
			image_file.clear(); // clear EOF flag
			image_file.seekg(0, std::ios::beg); // seek to the beginning of the file
			std::clog << "input_image size: " << image_file_size << std::endl;
			std::vector<byte> image_file_bytes; // will hold the file data
			image_file_bytes.resize(image_file_size); // prealocate vector
			image_file.read(reinterpret_cast<std::ifstream::char_type*>(image_file_bytes.data()), image_file_bytes.size()); // read the whole file into the vector
			image_file.close();
			is_png = (image_file_bytes.size() >= 8) && (!png_sig_cmp(image_file_bytes.data(), 0, 8)); // check header to see if it's a png
			if (is_png)
			{	
				std::clog << "Input image seems to be a png, will use libpng to load it" << std::endl;
				png_image libpng_img;
				memset(&libpng_img, 0, (sizeof libpng_img));
				libpng_img.version = PNG_IMAGE_VERSION;
				libpng_img.opaque = nullptr;
				if (png_image_begin_read_from_memory(&libpng_img, image_file_bytes.data(), image_file_bytes.size()) != 0)
				{
					if (libpng_img.format & PNG_FORMAT_FLAG_ALPHA) // convert to either rgba or rgb on read
					{
						libpng_img.format = PNG_FORMAT_RGBA;
						num_channels = 4;
					}
					else
					{
						libpng_img.format = PNG_FORMAT_RGB;
						num_channels = 3;
					}
					libpng_img.flags = 0;
					i_width  = libpng_img.width;
					i_height = libpng_img.height;
					image_bytes.resize(PNG_IMAGE_SIZE(libpng_img)); //preallocate the vector for the image data
					if (!image_bytes.empty())
					{
						if ((could_read_img = png_image_finish_read(&libpng_img, nullptr, image_bytes.data(), 0, nullptr) != 0))
						{
						std::clog << "Image successfully read using libpng" << std::endl;
						}
					}
				}
				if (!could_read_img)
				{
					could_read_img = false;
					if (libpng_img.warning_or_error != 0)
					{
						std::cerr << "libpng error: "  << libpng_img.message << std::endl;
					}
					else
					{
						std::cerr << "Unknown error reading image with libpng" << std::endl;
					}
					std::cerr << "Make sure the png file isn't corrupt" << std::endl;
				}
			}
			else
			{
				std::clog << "Input doesn't seem to be a png, will use stb_image to load it" << std::endl;
				num_channels = 3; // always get an rgb image
				byte* temp = stbi_load_from_memory(image_file_bytes.data(), image_file_bytes.size(), &i_width, &i_height, nullptr, STBI_rgb); // try to read image, stb will convert to rgb
				if ((could_read_img = temp != nullptr))
				{
					input_bytes = (static_cast<size_t>(i_width) * static_cast<size_t>(i_height) * static_cast<size_t>(num_channels)); // calculate used bytes
					image_bytes.resize(input_bytes);
					memcpy(image_bytes.data(), temp, image_bytes.size()); // move data to the image_bytes vector
					stbi_image_free(temp);
				}
				if (!could_read_img)
				{
					std::cerr << "Couldn't read image using stb_image, Make sure the image isn't corrupt" << std::endl;
				}
			}
		}
		else
		{
			std::cerr << "Couldn't open input image: \"" << input_image.string() << "\", " << std::strerror(errno) << std::endl;
		}
		if (could_read_img)
		{
			if (embed)
			{
				std::string input_filename = input_file.filename().string();
				if (!output_filename_set)
				{
					output_filename = path(input_filename + ".png");
				}
				std::error_code ec;
				bool file_exists = true;
				if (((!(file_exists = std::filesystem::exists(output_filename, ec))) || overwrite) && !static_cast<bool>(ec))
				{
					std::ifstream file_to_embed;
					file_to_embed.open(input_file, std::ios::binary);
					if(file_to_embed.is_open())
					{
						file_to_embed.ignore(std::numeric_limits<std::streamsize>::max());
						std::streamsize f_size = file_to_embed.gcount();
						u64 file_size = static_cast<u64>(f_size);
						file_to_embed.clear();
						file_to_embed.seekg(0, std::ios::beg);
						std::vector<byte> file_bytes;
						file_bytes.resize(f_size);
						file_to_embed.read(reinterpret_cast<std::ifstream::char_type *>(file_bytes.data()), file_bytes.size());
						file_to_embed.close();
						std::clog << "Size of file to embed: " << f_size << std::endl;
						u64 name_size = (u64)input_filename.length();
						std::clog << "name size: " << name_size << std::endl;
						/* the total_size that will be stored is 1 byte for the numbers of bits to use, 64 bits
						 * for the length of the name, the name of the file, another 64 bits for the size of the
						 * file, and the file itself.
						 */
						u64 total_size = 1 + sizeof(u64) + name_size + sizeof(u64) + file_size;
						u64 total_pixels = ImageLOL::required_pixels(total_size, bit_depth, num_channels);
						ImageLOL::dimensions output_size = ImageLOL::minimum_dimensions(total_pixels, i_width, i_height);
						u64 o_width, o_height;
						o_width = output_size.width;
						o_height = output_size.height;
						/*output size will be (close) to the minimum image with the same proportions as the original
						 *that can hold atleast total_size bytes, or the original size if it can already store enough
						 * bytes.
						 */
						std::clog << "ratio: " << static_cast<double>(o_width)/i_width << std::endl;
						std::clog << "output size: " << o_width << "," << o_height << std::endl;
						
						std::vector<byte> output_img;
						output_img.resize(o_width * o_height * num_channels);
						//initialize the image with an scaled version of the original one.
						for (u64 i = 0; i < o_height; i++)
						{
							for (u64 j = 0; j < o_width; j++)
							{
								output_img[(i * o_width + j) * num_channels]     = sample_image(image_bytes.data(), i_width, i_height, ((double)j/o_width),((double)i/o_height), num_channels, channels::r);
								output_img[(i * o_width + j) * num_channels + 1] = sample_image(image_bytes.data(), i_width, i_height, ((double)j/o_width),((double)i/o_height), num_channels, channels::g);
								output_img[(i * o_width + j) * num_channels + 2] = sample_image(image_bytes.data(), i_width, i_height, ((double)j/o_width),((double)i/o_height), num_channels, channels::b);
								if (num_channels == 4)
								{
									output_img[(i * o_width + j) * num_channels + 3] = sample_image(image_bytes.data(), i_width, i_height, ((double)j/o_width),((double)i/o_height), num_channels, channels::a);
								}
							}
						}
						ImageLOL::Image o_img(output_img.data(), o_width, o_height, num_channels);
						ImageLOL::ImageLOLWriter writer(o_img.getWriter(bit_depth));
						writer.write(input_filename);
						writer.write(file_bytes);
						//always writes the image to a png.
						png_image p_img;
						memset(&p_img, 0, (sizeof p_img));
						p_img.version = PNG_IMAGE_VERSION;
						p_img.opaque = nullptr;
						p_img.width = static_cast<uint32_t>(o_width);
						p_img.height = static_cast<int32_t>(o_height);
						p_img.format = ((num_channels < 4) ? PNG_FORMAT_RGB : PNG_FORMAT_RGBA);
						p_img.flags = 0;
						if (png_image_write_to_file(&p_img, output_filename.string().c_str(), 0, output_img.data(), 0, nullptr))
						{
							std::clog << "Successfully embedded file" <<std::endl;
							std::cout << output_filename.string() << std::endl;
							result = 0;
						}
						else
						{
							std::cerr << "Error while writing (through libpng): " << p_img.message << std::endl;
						}
					}
					else
					{
						std::cerr << "Couldn't open file to embed: \"" << input_file.string() << "\", " << std::strerror(errno) << std::endl;
					}
				}
				else
				{
					if (!ec)
					{
						if (file_exists)
						{
							std::cerr << "Output image: \"" << output_filename.string() << "\" exists, use --overwrite to overwrite it" << std::endl;
						}
					} else {
						std::cerr << "Error while checking if file \"" << output_filename.string() << "\" exists: " << ec.message() << std::endl;
					}
				}
			}
			else
			{
				ImageLOL::Image i_img(image_bytes.data(), i_width, i_height, num_channels);
				ImageLOL::ImageLOLReader reader(i_img.getReader());
				if ((reader.get_total_bytes() > 0) && (reader.get_bit_depth() > 0))
				{
					std::clog << "Image uses: " << static_cast<u64>(reader.get_bit_depth()) << " bits per byte" << std::endl;
					bit_depth = reader.get_bit_depth();
					std::string stored_filename(reader.read<std::string>());
					if (!output_filename_set)
					{
						output_filename = path(stored_filename);
					}
					std::error_code ec;
					bool file_exists = true;
					if (((!(file_exists = std::filesystem::exists(output_filename, ec))) || overwrite) && !static_cast<bool>(ec))
					{
						std::ofstream file;
						file.open(output_filename, std::ios::binary|std::ios::trunc);
						if (file.is_open())
						{
							std::vector<byte> file_data(reader.read<std::vector<byte>>());
							file.write(reinterpret_cast<const std::ofstream::char_type*>(file_data.data()), file_data.size());
							if (file.good())
							{
								std::clog << "Successfully extracted file" << std::endl;
								std::cout << stored_filename << std::endl;
								result = 0;
							}
							else
							{
								std::cerr << "Error while writing to: \"" << output_filename.string() << "\", " << std::strerror(errno) << std::endl;
							}
							file.close();
						}
						else
						{
							std::cerr << "Couldn't open output file: \"" << output_filename.string() << "\", " << std::strerror(errno) << std::endl;
						}
					}
					else
					{
						if (!ec)
						{
							if (file_exists)
							{
								std::cerr << "Output file: \"" << output_filename.string() << "\" exists, use --overwrite to overwrite it" << std::endl;
							}
						} else {
							std::cerr << "Error while checking if file \"" << output_filename.string() << "\" exists: " << ec.message() << std::endl;
						}
					}
				}
				else
				{
					std::cerr << "The image does not contain valid ImageLOL encoded data" << std::endl;
				}
			}
		}
		else
		{
			std::cerr << "Couldn't read input image" << std::endl;
		}
	}
	return result;
}
