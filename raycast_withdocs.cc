#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cassert>
#include <bitset> // used to print binaries to cout

// pack_color packs color values in a 4 byte integer
// << is a bitshift operation, so a is in the bit furthest to the right
// b is the next over to the left, then g, then r is first
// resulting in pretty weird looking numbers (and very often broken couts)
//
// to effectively print, i first tried to print just the binaries (since
// that's what is being written to the color anyway)
// I found out that casting <int> over r works, which confused me. 
// a uint8_t type is already an <int>...but I guess not. Oh well.
//
// Anyway, I decided that printing all of the color values you'd see on
// a color wheel (R, G, B, A) could be helpful to see what it's actually
// doing. Plus, looking into how PPM files are specified, they're written
// into int/char anyway as an ASCII file (denoted by P3 instead of P6
// down below)
//
// Using these two couts together you can see how both PPM specs differ, but
// being able to see it like this helped me understand how exactly the
// image is being created and interpreted.
uint32_t pack_color(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a=255) {
  std::bitset<8> bitr(r);
  std::bitset<8> bitb(b);
  std::bitset<8> bitg(g);
  std::bitset<8> bita(a);

  //std::cout << bita << bitb << bitg << bitr << std::endl;
  //std::cout << static_cast<int>(r) << "," << static_cast<int>(g) << "," << static_cast<int>(b) << "," << static_cast<int>(a) << std::endl;
  return (a<<24) + (b<<16) + (g<<8) + r;
}

// I'm equally curious as to what exactly this does. To me it looks like
// it's just checking if the color is maxed out, and if it is it sets a 
// bit to 1. So I'm just gonna debug print.
//
// Here's what I've learned: bitwise operators are pretty cool.
// It can pull directly from the byte and get what is specified.
// Pretty cool way to store colors.
void unpack_color(const uint32_t &color, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) {
  uint8_t rcolor = (color >>  0); // I'm expecting these to be bits since
  uint8_t gcolor = (color >>  8); // they are coming from a byte.
  uint8_t bcolor = (color >> 16); // So I should be able to use std::bitset
  uint8_t acolor = (color >> 24);

  std::bitset<8> bitr(rcolor);
  std::bitset<8> bitg(gcolor);
  std::bitset<8> bitb(bcolor);
  std::bitset<8> bita(acolor);

  //std::cout << bita << bitb << bitg << bitr << std::endl;

  r = (color >>  0) & 255;
  g = (color >>  8) & 255;
  b = (color >> 16) & 255;
  a = (color >> 24) & 255;
} 


void drop_ppm_image(const std::string filename, const std::vector<uint32_t> &image, const size_t w, const size_t h) {
  assert(image.size() == w*h);
  std::ofstream ofs(filename, std::ios::binary);
  ofs << "P6\n" << w << " " << h << "\n255\n";
  // This line looks weird, but it just tells the PPM format everything
  // that it needs to know. P6 is a magic number that specifies that
  // it's storing binary data. w h is the dimensions of the file. 255\n
  // denotes the end of the headerline. 

  for (size_t i = 0; i < h*w; ++i) {
    uint8_t r, g, b, a;
    unpack_color(image[i], r, g, b, a);
    //ofs << r << g << b;
    ofs << static_cast<char>(r) << static_cast<char>(g) << static_cast<char>(b);
  } // This loop goes through every single pixel in the &image vector 
    // It's pretty quick, getting through 262,144 pixels happens basically
    // instantly. I think it's printing it as a char so the binary print
    // doesn't fuck it up. I don't really know *why* doing it would fuck
    // it up. So...I'm gonna fuck it up!
    // Welp. I think it's some sort of safety netting, as this works
    // just fine (since it's expecting binary data anyway). 
    
  ofs.close();
}

int main() {
  const size_t win_w = 512;
  const size_t win_h = 512;
  std::vector<uint32_t> framebuffer(win_w*win_h, 255);
  // This framebuffer does the same thing as &image in drop_ppm_image
  // Although it starts with a default value of 255 across the entire
  // vector. Probably because of the default functionality of
  // checking if the uint8_t values are the same as 255.
  // I assume that, if I did no processing to the framebuffer,
  // the out.ppm file would just be white. Ah, interesting! The vector
  // only sets the value for red here. I wonder if there's any real reason
  // for that. Maybe just in case there's an error with pack_color, so that
  // the image displays at all?

  for (size_t j = 0; j<win_h; j++) {
    for (size_t i = 0; i<win_w; i++) {
      uint8_t r = 255*j/float(win_h);
      uint8_t g = 255*i/float(win_w);
      uint8_t b = 0;
      
      framebuffer[i+j*win_w] = pack_color(r, g, b);
    }
  }

  drop_ppm_image("./out.ppm", framebuffer, win_w, win_h);

  return 0;
}
