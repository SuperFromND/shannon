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
#include <zip.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include "font.h"

using std::string;

// SDL boilerplate shenanagains
SDL_Window* window;
SDL_Renderer* renderer;
int width  = 720;
int height = 480;
int x, y;

SDL_Surface* font;
SDL_Texture* font_texture;

// touchHLE-specific stuff
// TODO: scrape app name, version, and icon instead of just placeholders
struct app {
    std::string name = "Unknown App";
    std::string filename;
    std::string filepath;
    std::string version = "Unknown";
    SDL_Texture* icon;
};

const std::filesystem::path apps{"touchHLE_apps"};
const std::filesystem::path icon_cache{"shannon_icon_cache"};

std::vector<app> apps_list;
int apps_count;
int scroll_offset = 0;

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

    // clear out font just in case
    SDL_FreeSurface(font);
    SDL_DestroyTexture(font_texture);

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

void extract_icon(const char* file, const char* name) {
    struct zip_t *zip = zip_open(file, 0, 'r');
    zip_entry_open(zip, "iTunesArtwork");
    zip_entry_fread(zip, name);
    zip_entry_close(zip);
    zip_close(zip);
}

app extract_plist_metadata(const char* file) {
    app output;
    struct zip_t *zip = zip_open(file, 0, 'r');
    std::string info_plist_path = "dummy";

    // the path to get a given IPA's info.plist is NOT trivial or predictable
    // so the best way to find it, unfortunately, is to just loop over the
    // entire file until we find the Info.plist file
    int n = zip_entries_total(zip);

    for (int i = 0; i < n; i++) {
        zip_entry_openbyindex(zip, i);
        std::string name = zip_entry_name(zip);

        if (name.find("Info.plist") != std::string::npos) {
            info_plist_path = name;
            zip_entry_close(zip);
            break;
        }

        zip_entry_close(zip);
    }

    zip_entry_open(zip, info_plist_path.c_str());

    // read contents of plist into buffer that we can do stuff with
    void *buf = NULL;
    size_t bufsize;
    zip_entry_read(zip, &buf, &bufsize);

    // TODO: parse the plist here. Probably need to find a library to read Plist binary because I REALLY don't want to write my own. (Apple's code is open-source but I'm unsure of its licensing and it looks to use a lot of extra libs :<)

    zip_entry_close(zip);
    zip_close(zip);

    return output;
}

void display_background() {
    // just for fun :)
    SDL_SetRenderDrawColor(renderer, 1, 0, 2, 255);
    SDL_RenderClear(renderer);
}

void display_list() {
    if (apps_count <= 0) {
        draw_text("Could not find any apps. =(", width/2, height/2, 1, 0);
    } else {
        SDL_Rect icon;
        icon.w = icon.h = 57;
        icon.x = 2;

        // draws underlay
        if (y < (apps_count*64) + (scroll_offset*64)) {
            SDL_Rect app_box;
            app_box.x = 0;
            app_box.y = (y/64) * 64;
            app_box.w = width;
            app_box.h = 64;

            SDL_SetRenderDrawColor(renderer, 96, 12, 32, 128);
            SDL_RenderFillRect(renderer, &app_box);
        }

        for (int i = 0; i < apps_count; i++) {
            SDL_Color version_col = {255, 96, 96};

            int app_y_pos = (scroll_offset*64) + (i*64) + 2;

            //draw_text(apps_list[i].name, 64, app_y_pos);
            draw_text(apps_list[i].filename, 64, app_y_pos + 16, 1, 1, width, {127, 127, 160});
            //draw_text("iOS version " + apps_list[i].version, 64, app_y_pos + 32, 1, 1, width, version_col);

            icon.y = app_y_pos;

            // placeholder icon
            SDL_SetRenderDrawColor(renderer, i*16, i*32, i*64, 255);
            SDL_RenderFillRect(renderer, &icon);
            draw_text(std::to_string(i), 2, icon.y);
            SDL_SetTextureScaleMode(apps_list[i].icon, SDL_ScaleModeLinear);
            SDL_RenderCopy(renderer, apps_list[i].icon, NULL, &icon);
        }
    }
}

void scan_apps() {
    if (!std::filesystem::is_directory(apps)) {
        printf("[!] The apps directory (%s) couldn't be found!\n", apps.string().c_str());
        return;
    }

    if (!std::filesystem::is_directory(icon_cache)) {
        printf("The icon cache directory (%s) couldn't be found! Creating one...\n", icon_cache.string().c_str());
        std::filesystem::create_directory(icon_cache);
    }

    for (auto& entry: std::filesystem::directory_iterator(apps)) {
        if (entry.path().extension() == ".ipa") {
            app app_entry;
            app_entry.name = "Unknown App";
            app_entry.version = "Unknown";
            app_entry.filename = entry.path().filename().string();
            app_entry.filepath = entry.path().string();

            SDL_Texture* temp;
            std::string cache_path = icon_cache.string() + "/" + app_entry.filename + ".png";

            temp = IMG_LoadTexture(renderer, cache_path.c_str());

            if (temp == NULL) {
                printf("[!]: %s\n", IMG_GetError());
                extract_icon(app_entry.filepath.c_str(), cache_path.c_str());
                temp = IMG_LoadTexture(renderer, cache_path.c_str());
            }

            app_entry.icon = temp;

            extract_plist_metadata(app_entry.filepath.c_str());

            apps_list.push_back(app_entry);
        }
    }

    apps_count = apps_list.size();
}

void reload_app_icons() {
    // called when recreating the window
    for (int i = 0; i < apps_count; i++) {
        SDL_DestroyTexture(apps_list[i].icon);
        SDL_Texture* temp;
        std::string cache_path = icon_cache.string() + "/" + apps_list[i].filename + ".png";

        temp = IMG_LoadTexture(renderer, cache_path.c_str());

        if (temp == NULL) {
            printf("[!]: %s\n", IMG_GetError());
            extract_icon(apps_list[i].filepath.c_str(), cache_path.c_str());
            temp = IMG_LoadTexture(renderer, cache_path.c_str());
        }

        apps_list[i].icon = temp;
    }
}

void launch_app() {
    int app = (y/64) - scroll_offset;
    if (app > apps_count-1) {return;}
    std::string command = "touchHLE.exe \"" + apps_list[app].filepath + "\"";

    system(command.c_str());
}

bool init() {
    // initialize SDL stuff
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("[!] Error initializing SDL: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "TRUE");

    // create window
    window = SDL_CreateWindow("Shannon: A Basic TouchHLE Frontend", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_RESIZABLE);

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
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

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

    scan_apps();

    while (program_running) {
        while (SDL_PollEvent(&evt) != 0) {
            switch (evt.type) {
                case SDL_QUIT: program_running = false; break;

                case SDL_WINDOWEVENT:
                    if (evt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        SDL_RenderClear(renderer);
                        SDL_GetWindowSize(window, &width, &height);
                        scroll_offset = 0;
                    }
                    break;

                case SDL_MOUSEWHEEL:
                    scroll_offset = fmax(fmin(0, scroll_offset + evt.wheel.y), -apps_count + ((float)(apps_count * 64) / height));
                    break;

                case SDL_MOUSEMOTION:
                    SDL_GetMouseState(&x, &y);
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    if (x > width || x < 0 || y > height || y < 0 || apps_count == 0) {break;}
                    if (y > (apps_count*64) + (scroll_offset*64)) {break;}
                    if (evt.button.button == SDL_BUTTON_LEFT) {
                        SDL_DestroyRenderer(renderer);
                        SDL_DestroyWindow(window);
                        launch_app();
                        init();
                        reload_app_icons();
                    }
                    break;
            }
        }

        display_background();
        display_list();
        //draw_text(std::to_string(x), width, 0,  1, -1, width, {255, 255, 96});
        //draw_text(std::to_string(y), width, 16, 1, -1, width, {255, 255, 96});

        SDL_RenderPresent(renderer);
    }

    kill();
    return 0;
}