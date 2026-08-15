#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <cassert>
#include <sstream>
#include <iomanip>

#include "map.h"
#include "utils.h"
#include "player.h"
#include "sprite.h"
#include "framebuffer.h"
#include "textures.h"

int wall_x_texcoord(const float hitx, const float hity, Texture &tex_walls) {
  float x = hitx - floor(hitx+.5);
  float y = hity - floor(hity+.5);
  int tex = x*tex_walls.size;
  if (std::abs(y)>std::abs(x))
    tex = y*tex_walls.size;
  if (tex<0)
    tex += tex_walls.size;
  assert(tex>=0 && tex<(int)tex_walls.size);
  return tex;
}

void map_show_sprite(Sprite &sprite, FrameBuffer &fb, Map &map) {
  const size_t rect_w = fb.w/(map.w*2);
  const size_t rect_h = fb.h/map.h;
  fb.draw_rectangle(sprite.x*rect_w-3, sprite.y*rect_h-3, 6, 6, pack_color(255,0,0));
}

void draw_sprite(Sprite &sprite, std::vector<float> &depth_buffer, FrameBuffer &fb, Player &player, Texture &tex_sprites) {
  // absolute direction from player to sprite - radians
  float sprite_dir = atan2(sprite.y - player.y, sprite.x - player.x);
  while (sprite_dir - player.a > M_PI) sprite_dir -= 2*M_PI;

  while (sprite_dir - player.a < -M_PI) sprite_dir += 2*M_PI;

  size_t sprite_screen_size = std::min(1000, static_cast<int>(fb.h/sprite.player_dist));
  
  int h_offset = (sprite_dir - player.a)/player.fov*(fb.w/2) + (fb.w/2)/2 - tex_sprites.size/2;
  int v_offset = fb.h/2 - sprite_screen_size/2;

  for (size_t i=0; i<sprite_screen_size; i++) {
    if (h_offset+int(i)<0 || h_offset+i>=fb.w/2) continue;
    if (depth_buffer[h_offset+i]<sprite.player_dist) continue;
    for (size_t j=0; j<sprite_screen_size; j++) {
      if (v_offset+int(j)<0 || v_offset+j>=fb.h) continue;
      uint32_t color = tex_sprites.get(i*tex_sprites.size/sprite_screen_size, j*tex_sprites.size/sprite_screen_size, sprite.tex_id);
      uint8_t r,g,b,a;
      unpack_color(color, r, g, b, a);
      if (a>128)
        fb.set_pixel(fb.w/2 + h_offset+i, v_offset+j, color);
    }
  }
}

void render(FrameBuffer &fb, Map &map, Player &player, std::vector<Sprite> &sprites, Texture &tex_walls, Texture &tex_monst) {
  fb.clear(pack_color(255, 255, 255));
  const size_t rect_w = fb.w/(map.w*2);
  const size_t rect_h = fb.h/map.h;

  for (size_t j=0; j<map.h; j++) {
    for (size_t i=0; i<map.w; i++) {
      // draw the grid, rectangle size is always the same, that only thing
      // that changes is the x
      if (map.is_empty(i, j)) {
        fb.draw_rectangle(i*rect_w, j*rect_h, 1, fb.h, pack_color(255, 0, 255));
        fb.draw_rectangle(i*rect_w, j*rect_h, rect_w, 1, pack_color(255, 0, 255));
        continue;
      }
      size_t rect_x = i*rect_w;
      size_t rect_y = j*rect_h;
      size_t texid = map.get(i, j);
      assert(texid<tex_walls.count);
      fb.draw_rectangle(rect_x, rect_y, rect_w, rect_h, tex_walls.get(0, 0, texid));
    }
  }
  std::vector<float> depth_buffer(fb.w/2, 1e3);
  for (size_t i=0; i<fb.w/2; i++) {
    float angle = player.a-player.fov/2 + player.fov*i/float(fb.w/2);
    for (float t=0; t<20; t+=0.01) { // normally t<20
      float x = player.x + t*cos(angle);
      float y = player.y + t*sin(angle);
      fb.set_pixel(x*rect_w, y*rect_h, pack_color(160, 160, 160));

      if (map.is_empty(x, y)) continue;

      size_t texid = map.get(x, y);
      assert(texid<tex_walls.count);
      float dist = t*cos(angle-player.a);
      depth_buffer[i] = dist;
      size_t column_height = fb.h/dist;
      int x_texcoord = wall_x_texcoord(x, y, tex_walls);
      std::vector<uint32_t> column = tex_walls.get_scaled_column(texid, x_texcoord, column_height); // column textures
      int pix_x = i + fb.w/2;
      for (size_t j=0; j<column_height; j++) {
        int pix_y = j + fb.h/2 - column_height/2; 
        if (pix_y>=0 && pix_y<(int)fb.h) {
          fb.set_pixel(pix_x, pix_y, column[j]); // This handles drawing in
                                                 // 3D.
        }
      }
      break;
    }
  }

  for (size_t i=0; i<sprites.size(); i++) {
    sprites[i].player_dist = std::sqrt(pow(player.x - sprites[i].x, 2) + pow(player.y - sprites[i].y, 2));
  }
  std::sort(sprites.begin(), sprites.end());

  for (size_t i=0; i<sprites.size(); i++) {
    map_show_sprite(sprites[i], fb, map);
    draw_sprite(sprites[i], depth_buffer, fb, player, tex_monst);
  }
}
/*(
int main() {
  drop_ppm_image("./out.ppm", fb.img, fb.w, fb.h);
  return 0;
}
*/
