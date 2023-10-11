/*
*   This program/source code is licensed under the MIT License:
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included in all
*   copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*
*/

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include "font.h"

using std::string;

// SDL boilerplate shenanagains
SDL_Window* window;
SDL_Renderer* renderer;
int width  = 854;
int height = 480;

SDL_Surface* font;
SDL_Texture* font_texture;

void load_font() {
    Uint32 rmask, gmask, bmask, amask;

    // set color masks based on SDL endianness
    if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
        rmask = 0xff000000;
        gmask = 0x00ff0000;
        bmask = 0x0000ff00;
        amask = 0x000000ff;
    } else {
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = 0xff000000;
    }

    font = SDL_CreateRGBSurfaceFrom((void*)fallback_font.pixel_data, fallback_font.width, fallback_font.height, fallback_font.bytes_per_pixel*8, fallback_font.bytes_per_pixel*fallback_font.width, rmask, gmask, bmask, amask);
    font_texture = SDL_CreateTextureFromSurface(renderer, font);
    return;
}

void draw_text(string text, int x = 0, int y = 0, int scale = 1, int align = 1, int max_width = width, SDL_Color mul = {255, 255, 255}) {
    // Bitmap monospaced font-drawing function, supports printable ASCII only
    // ----------------------------------------------------------
    // text: a std string,          e.g. "Hello World"
    // x, y: x and y coordinates,   e.g. "320, 240"
    // scale: scaling factor,       e.g. "2" for double-size text
    // align: alignment setting     e.g. 0 for centered, 1 for right-align, -1 for left-align
    // max_width: max width that text can occupy; set to 0 to disable
    // mul: SDL_Color to multiply font texture with (in other words, the text color)

    // printable ASCII (use this string for making new fonts):
    //  !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~

    // skips the entire function if the font happens to have not loaded for whatever reason
    // prevents a crash
    if (font == NULL) {
        return;
    }

    SDL_SetTextureScaleMode(font_texture, SDL_ScaleModeNearest);
    SDL_SetTextureColorMod(font_texture, mul.r, mul.g, mul.b);
    SDL_Rect src;
    SDL_Rect dest;

    int char_width  = font->w/95;
    int char_height = font->h;
    int scaled_char_width = char_width;
    int text_size = text.size();

    // resizes characters if need be
    if (max_width < text_size * (char_width * scale) && max_width != 0) {
        scaled_char_width = max_width / text_size;
    } else {
        scaled_char_width *= scale;
    }

    for (int i = 0; i < text_size; ++i) {
        // get ASCII value of current character
        // we also define char properties here due to some resizing shenanagains later
        int char_value = text[i] - 32;
        int align_offset = 0;

        // get character coords in source image
        // width and height are 1 character
        src.x = (char_value * char_width);
        src.y = 0;
        src.w = char_width;
        src.h = char_height;

        // determine offset value to use
        if (align >= 1) {align_offset = 0;}
        else if (align == 0) {align_offset = ((text_size * scaled_char_width)/2) * -1;}
        else if (align <= -1) {align_offset = (text_size * scaled_char_width) * -1;}

        // get x and y coords, offset by current character count and align/scale factors
        // width and height bound-box get scaled here as well
        dest.x = x + (i * scaled_char_width) + align_offset;
        dest.y = y;
        dest.w = scaled_char_width;
        dest.h = char_height * scale;

        // skip character if it's out of view
        if (dest.x > width || dest.x < -dest.w || dest.y > height || dest.y < -dest.h) {continue;}

        SDL_RenderCopy(renderer, font_texture, &src, &dest);
    }
    return;
}

bool init() {
    // initialize SDL stuff
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("[!] Error initializing SDL: %s\n", SDL_GetError());
        return false;
    }
    
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "TRUE");

    // create window
    window = SDL_CreateWindow("Example Program", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_RESIZABLE);

    if (window == NULL) {
        printf("[!] Error creating window: %s\n", SDL_GetError());
        return false;
    }
    
    // create renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    if (renderer == NULL) {
        printf("[!] Error creating renderer: %s\n", SDL_GetError());
        return false;
    }
    
    load_font();
    
    return true;
}

void kill() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char* args[]) {
    bool program_running = true;
    SDL_Event evt;
    
    if (!init()) {program_running = false; return 1;}
    
    while (program_running) {
        while (SDL_PollEvent(&evt) != 0) {
            switch (evt.type) {
                case SDL_QUIT: program_running = false; break;
                
                case SDL_WINDOWEVENT:
                    if (evt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        SDL_RenderClear(renderer);
                        SDL_GetWindowSize(window, &width, &height);
                    }
                break;
            }
        }
        
        SDL_SetRenderDrawColor(renderer, 12, 8, 16, 255);
        SDL_RenderClear(renderer);
        draw_text("hello world!");
        SDL_RenderPresent(renderer);
    }
    
    kill();
    return 0;
}