// raycast_withdocs.cc
// This is the version with notes. It has a lot of entirely
// unorganized notes that are placed above whatever function
// I'm currently messing with.
//
// As I follow this project along, I'm aware that these notes are
// probably just going to become entirely illegible. That's okay.

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cassert>
#include <cmath> // for sin, cos
#include <bitset> // used to print binaries to cout
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

// draw rectangle unsurprisingly draws a rectangle
// it specifically draws it on top of another image which is why it needs 
// the dimensions of the image and the image vector
// x and y are the coordinates, w and h are the width and height
// The only part I don't particularly understand is: how does this draw a 
// rectangle with only an x and a y? 
// img[cx + cy*img_w] is another multi-dimensional array crammed into a one-dim
// so that part makes sense. 
// Ok, after couting the cx+cy*img_w I get it. This is going pixel by pixel and 
// changing drawing the rectangle that way, under the constraints of the width
// and the height. This is because of the double for loop, only going the width
// and the height, which sets the x y as starting in the bottom left (which is
// why cx and cy exist, these are the colored x and y positions). img[cx+cy*img_w]
// is set to the color.
void draw_rectangle(std::vector<uint32_t> &img, const size_t img_w, const size_t img_h, const size_t x, const size_t y, const size_t w, const size_t h, const uint32_t color) {
  assert(img.size()==img_w*img_h);
  for (size_t i=0; i<w; i++) {
    for (size_t j=0; j<h; j++) {
      size_t cx = x+i;
      size_t cy = y+j;
      if (cx>=img_w || cy>=img_h) continue;
      img[cx + cy*img_w] = color;
    }
  }
}

bool load_texture(const std::string filename, std::vector<uint32_t> &texture, size_t &text_size, size_t &text_cnt) {
  int nchannels = -1, w, h;
  unsigned char *pixmap = stbi_load(filename.c_str(), &w, &h, &nchannels, 0);
  if (!pixmap) {
    std::cerr << "Error: can not load the textures" << std::endl;
    return false;
  }

  if (4!=nchannels) {
    std::cerr << "Error: the texture must be a 32 bit image" << std::endl;
    stbi_image_free(pixmap);
    return false;
  }

  text_cnt = w/h;
  text_size = w/text_cnt;
  if (w!=h*int(text_cnt)) {
    std::cerr << "Error: the texture file must contain N square textures packed horizontally" << std::endl;
    stbi_image_free(pixmap);
    return false;
  }

  texture = std::vector<uint32_t>(w*h);
  for (int j=0; j<h; j++) {
    for (int i=0; i<w; i++) {
      uint8_t r = pixmap[(i+j*w)*4+0];
      uint8_t g = pixmap[(i+j*w)*4+1];
      uint8_t b = pixmap[(i+j*w)*4+2];
      uint8_t a = pixmap[(i+j*w)*4+3];
      texture[i+j*w] = pack_color(r, g, b, a);
    }
  }
  stbi_image_free(pixmap);
  return true;
}

std::vector<uint32_t> texture_column(const std::vector<uint32_t> &img, const size_t texsize, const size_t ntextures, const size_t texid, const size_t texcoord, const size_t column_height) {
    const size_t img_w = texsize*ntextures;
    const size_t img_h = texsize;
    assert(img.size()==img_w*img_h && texcoord<texsize && texid<ntextures);
    std::vector<uint32_t> column(column_height);
    for (size_t y=0; y<column_height; y++) {
        size_t pix_x = texid*texsize + texcoord;
        size_t pix_y = (y*texsize)/column_height;
        column[y] = img[pix_x + pix_y*img_w];
    }
    return column;
}

int main() {
  const size_t win_w = 1024;
  const size_t win_h = 512;
  std::vector<uint32_t> framebuffer(win_w*win_h, pack_color(255, 255, 255));
  // This framebuffer does the same thing as &image in drop_ppm_image
  // Although it starts with a default value of 255 across the entire
  // vector. Probably because of the default functionality of
  // checking if the uint8_t values are the same as 255.
  // I assume that, if I did no processing to the framebuffer,
  // the out.ppm file would just be white. Ah, interesting! The vector
  // only sets the value for red here. I wonder if there's any real reason
  // for that. Maybe just in case there's an error with pack_color, so that
  // the image displays at all? Now the framebuffer gets initialized with white
  // instead of red. How nice.

  const size_t map_w = 16;
  const size_t map_h = 16;

  // This map array is weird. I'm not entirely sure how it works.
  // From what I do understand, the blank spaces are filled in with
  // color, the rest aren't filled in. I'm not sure specifically
  // what the numbers do. After testing by changing all the numbers to
  // 0s or 1s, the map is identical, but it might be setting something
  // up for later. 
  // The numbers set up the texture value on the walls!
  const char map[] = "0000222222220000"\
                     "1              0"\
                     "1      11111   0"\
                     "1     0        0"\
                     "0     0  1110000"\
                     "0     3        0"\
                     "0   10000      0"\
                     "0   3   11100  0"\
                     "5   4   0      0"\
                     "5   4   1  00000"\
                     "0       1      0"\
                     "2       1      0"\
                     "0       0      0"\
                     "0 0000000      0"\
                     "0              0"\
                     "0002222222200000";
  assert(sizeof(map) == map_w*map_h+1);

  // Pretty sure this values are arbitrary currently.
  float player_x = 3.456;
  float player_y = 2.345;
  float player_a = 1.523;
  const float fov = M_PI/3.;

  std::vector<uint32_t> walltext;
  size_t walltext_size;
  size_t walltext_cnt;
  if (!load_texture("./walltext.png", walltext, walltext_size, walltext_cnt)) {
      std::cerr << "Failed to load wall textures" << std::endl;
      return -1;
  }

  // Going to keep this part just because I want to. Maybe I'll change my mind.
  /*for (size_t j = 0; j<win_h; j++) {
    for (size_t i = 0; i<win_w; i++) {
      uint8_t r = 255*j/float(win_h);
      uint8_t g = 255*i/float(win_w);
      uint8_t b = 0;
      
      framebuffer[i+j*win_w] = pack_color(r, g, b);
    }
  }*/

  // This map drawing section just makes sense. Checks if the map char[] has blanks,
  // if a space is blank, move on to until you run into a non-blank char. 
  // If the char isn't blank, draw a rectangle of width,height from
  // rect_x to rect_y. 
  const size_t rect_w = win_w/(map_w*2); // map_w is doubled to compensate for the larger win_w
  const size_t rect_h = win_h/map_h;

  /*for (size_t frame=0; frame<360; frame++) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(5) << frame << ".ppm";
    player_a += 2*M_PI/360;

    framebuffer = std::vector<uint32_t>(win_w*win_h, pack_color(255, 255, 255));
    */
    for (size_t j=0; j<map_h; j++) {
      for (size_t i=0; i<map_w; i++) {
        if (map[i+j*map_w]==' ') continue; // skips blanks
        size_t rect_x = i*rect_w;
        size_t rect_y = j*rect_h;
        size_t texid = map[i+j*map_w] - '0';
        assert(texid<walltext_cnt);
        draw_rectangle(framebuffer, win_w, win_h, rect_x, rect_y, rect_w, rect_h, walltext[texid*walltext_size]);
      }
    }
    
    for (size_t i=0; i<win_w/2; i++) {
    // so why this angle?
    // fov is pi divided by 3 (so a little bit more than 1)
    // the math looks kinda like this 
    // (1.523 - (1/6pi) + (1/3pi * (i/win_w))) 
    // This should always scan from left to right in around a 45 degree angle
      float angle = player_a-fov/2 + fov*i/float(win_w/2);
    // Initial ray logic
    // Trying see the direction a player is "looking". Since there's no control
    // over the player right now, the direction is somewhat arbitrary, but this
    // is still the method to find it.
    // From my understanding, t is the distance from the player coordinates
    // and pix_x and pix_y coordinates. cos and sin are used to remain on the 
    // hypotenuse, since it's going to change whenever the player does change
    // direction.
      for (float t=0; t<20; t+=.01) {
        float cx = player_x + t*cos(angle);
        float cy = player_y + t*sin(angle);

        int pix_x = cx*rect_w;
        int pix_y = cy*rect_h;
        framebuffer[pix_x + pix_y*win_w] = pack_color(160, 160, 160);

        if (map[int(cx)+int(cy)*map_w]!=' ') { // if we touch a wall, visualize it
          size_t texid = map[int(cx)+int(cy)*map_w] - '0';
          assert(texid<walltext_cnt);

          // cos is used to determine the proper angle of difference
          // between angle and player_a, instead of letting it be entirely
          // based on t. 
          size_t column_height = win_h/(t*cos(angle-player_a));
        // the win_w/2+i here basically splits the image into two views:
        // one view is the map, the other view is the 3D view
        // The way this this 3d works is interesting. 
          float hitx = cx - floor(cx+.5);
          float hity = cy - floor(cy+.5);
          int x_texcoord = hitx*walltext_size;
          if (std::abs(hity)>std::abs(hitx)) {
              x_texcoord = hity*walltext_size;
          }

          if (x_texcoord<0) x_texcoord += walltext_size;

          std::vector<uint32_t> column = texture_column(walltext, walltext_size, walltext_cnt, texid, x_texcoord, column_height);
          pix_x = win_w/2+i;
          for (size_t j=0; j<column_height; j++) {
              pix_y = j + win_h/2-column_height/2;
              if (pix_y<0 || pix_y>=(int)win_h) continue;
              framebuffer[pix_x + pix_y*win_w] = column[j];
          }

          break;
        }
      }
    }
    
    drop_ppm_image("./out.ppm", framebuffer, win_w, win_h);
  //}

  return 0;
}
