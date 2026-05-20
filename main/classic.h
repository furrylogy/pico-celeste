#ifndef CLASSIC_H
#define CLASSIC_H
#include "pico8sim.h"

#define MAX_OBJECTS_COUNT 50

typedef struct room_s{
    int16_t x;
    int16_t y;
}room_t;

typedef struct flip_s{
    uint8_t x;
    uint8_t y;
}flip_t;

typedef struct vec2d_s{
    fix16_t x;
    fix16_t y;
}vec2d_t;

typedef struct box2d_s{
    fix16_t x;
    fix16_t y;
    fix16_t w;
    fix16_t h;
}box2d_t;

typedef struct obj_s obj_t;

typedef struct type_s{
    uint8_t tile;
    uint8_t if_not_fruit;
    size_t size; //需要分配多大的内存

    void (*init)(obj_t* obj);
    void (*update)(obj_t* obj);
    void (*draw)(obj_t* obj);
}type_t;

typedef struct obj_s{
    type_t* type;
    uint8_t valid;

    uint8_t collideable;
    uint8_t solids;
    
    uint8_t spr;
    flip_t flip;
    
    fix16_t x;
    fix16_t y;
    box2d_t hitbox;

    vec2d_t spd;
    vec2d_t rem;

    uint8_t (*is_solid)(obj_t* obj,fix16_t ox,fix16_t oy);
    uint8_t (*is_ice)(obj_t* obj,fix16_t ox,fix16_t oy);
    obj_t* (*collide)(obj_t* obj,type_t* type,fix16_t ox,fix16_t oy);
    uint8_t (*check)(obj_t* obj,type_t* type,fix16_t ox,fix16_t oy);
    void (*move)(obj_t* obj,fix16_t ox,fix16_t oy);
    void (*move_x)(obj_t* obj,fix16_t amount,int16_t start);
    void (*move_y)(obj_t* obj,fix16_t amount);

}obj_t;

typedef struct cloud_s{
    fix16_t x;
    fix16_t y;
    fix16_t spd;
    fix16_t w;
}cloud_t;

typedef struct particle_s{
    fix16_t x;
    fix16_t y;
    fix16_t s;  //scale
    fix16_t spd;
    fix16_t off;
    uint8_t c;
}particle_t;

typedef struct dead_particle_s{
    fix16_t x;
    fix16_t y;
    fix16_t t;
    vec2d_t spd;
    uint8_t alive;
}dead_particle_t;

typedef struct hair_s{
    fix16_t x;
    fix16_t y;
    fix16_t size;
}hair_t;

typedef struct playerobj_s{
    obj_t obj;

    hair_t hair[5];
} playerobj_t;

typedef struct player_s{
    playerobj_t obj;
    
    uint8_t p_jump;
    uint8_t p_dash;
    int16_t grace;
    int16_t jbuffer;
    int16_t djump;
    int16_t dash_time;
    int16_t dash_effect_time;
    vec2d_t dash_target;
    vec2d_t dash_accel;
    int16_t spr_off;
    uint8_t was_on_ground;
}player_t;

typedef struct player_spawn_s{
    playerobj_t obj;

    vec2d_t target;
    int16_t state;
    int16_t delay;
}player_spawn_t;

typedef struct spring_s{
    obj_t obj;

    int16_t hide_in;
    int16_t hide_for;
    int16_t delay;
}spring_t;

typedef struct balloon_s{
    obj_t obj;

    fix16_t offset;
    fix16_t start;
    int16_t timer;
}balloon_t;

typedef struct fall_floor_s{
    obj_t obj;

    int16_t state;
    uint8_t solid;
    int16_t delay;
}fall_floor_t;

typedef struct smoke_s{
    obj_t obj;

    int16_t spr_off;
}smoke_t;

typedef struct fruit_s{
    obj_t obj;

    fix16_t start;
    fix16_t off;
}fruit_t;

typedef struct fly_fruit_s{
    obj_t obj;

    fix16_t start;
    uint8_t fly;
    fix16_t step;
    int16_t sfx_delay;
}fly_fruit_t;

typedef struct lifeup_s{
    obj_t obj;

    int16_t duration;
    int16_t flash;
}lifeup_t;

typedef struct fake_wall_s{
    obj_t obj;

}fake_wall_t;

typedef struct key_s{
    obj_t obj;

    fix16_t spr;
}key_t;

typedef struct chest_s{
    obj_t obj;

    fix16_t start;
    int16_t timer;
}chest_t;

typedef struct platform_s{
    obj_t obj;

    fix16_t dir;
    fix16_t last;
}platform_t;

typedef struct message_s{
    obj_t obj;

    uint16_t last;
    uint16_t index;
}message_t;


typedef struct bc_particle_s{
    fix16_t x;
    fix16_t y;
    fix16_t h;
    fix16_t spd;
}bc_particle_t;

typedef struct big_chest_s{
    obj_t obj;

    int16_t state;
    int16_t timer;
    bc_particle_t particles[50];
    int16_t particle_count;
}big_chest_t;

typedef struct orb_s{
    obj_t obj;


}orb_t;

typedef struct flag_s{
    obj_t obj;

    uint8_t show;
    int16_t score;
}flag_t;

typedef struct room_title_s{
    obj_t obj;

    int16_t delay;
}room_title_t;

enum k_key_e{
    k_left=0,
    k_right,
    k_up,
    k_down,
    k_jump,
    k_dash
}k_key_t;

void title_screen();
void begin_game();
uint16_t level_index();
uint8_t is_title();

void clouds_init();
void particles_init();

void player_init(obj_t* obj);
void player_update(obj_t* obj);
void player_draw(obj_t* obj);

void psfx(uint8_t num);
void create_hair(obj_t* obj);
void set_hair_color(int16_t djump);
void draw_hair(obj_t* obj,fix16_t facing);
void unset_hair_color();

void player_spawn_init(obj_t* obj);
void player_spawn_update(obj_t* obj);
void player_spawn_draw(obj_t* obj);

void spring_init(obj_t* obj);
void spring_update(obj_t* obj);

void break_spring(obj_t* obj);

void balloon_init(obj_t* obj);
void balloon_update(obj_t* obj);
void balloon_draw(obj_t* obj);

void fall_floor_init(obj_t* obj);
void fall_floor_update(obj_t* obj);
void fall_floor_draw(obj_t* obj);

void break_fall_floor(obj_t* obj);

void smoke_init(obj_t* obj);
void smoke_update(obj_t* obj);

void fruit_init(obj_t* obj);
void fruit_update(obj_t* obj);

void fly_fruit_init(obj_t* obj);
void fly_fruit_update(obj_t* obj);
void fly_fruit_draw(obj_t* obj);

void lifeup_init(obj_t* obj);
void lifeup_update(obj_t* obj);
void lifeup_draw(obj_t* obj);

void fake_wall_update(obj_t* obj);
void fake_wall_draw(obj_t* obj);

void key_init(obj_t* obj);
void key_update(obj_t* obj);

void chest_init(obj_t* obj);
void chest_update(obj_t* obj);
void chest_draw(obj_t* obj);

void platform_init(obj_t* obj);
void platform_update(obj_t* obj);
void platform_draw(obj_t* obj);

void message_draw(obj_t* obj);

void big_chest_init(obj_t* obj);
void big_chest_draw(obj_t* obj);

void orb_init(obj_t* obj);
void orb_draw(obj_t* obj);

void flag_init(obj_t* obj);
void flag_draw(obj_t* obj);

void room_title_init(obj_t* obj);
void room_title_draw(obj_t* obj);

obj_t* init_object(type_t* type,fix16_t x,fix16_t y);
void destroy_object(obj_t* obj);
void kill_player(obj_t* obj);

void restart_room();
void next_room();
void load_room(int16_t x,int16_t y);

void draw_object(obj_t* obj);
void draw_time(fix16_t x,fix16_t y);

fix16_t clamp(fix16_t val,fix16_t a,fix16_t b);
fix16_t appr(fix16_t val,fix16_t target,fix16_t amount);
fix16_t sign(fix16_t v);
uint8_t maybe();

fix16_t p8sin(fix16_t x);
fix16_t p8cos(fix16_t x);

uint8_t solid_at(fix16_t x,fix16_t y,fix16_t w,fix16_t h);
uint8_t ice_at(fix16_t x,fix16_t y,fix16_t w,fix16_t h);
uint8_t tile_flag_at(fix16_t x,fix16_t y,fix16_t w,fix16_t h,uint8_t flag);
uint8_t tile_at(int16_t x,int16_t y);
uint8_t spikes_at(fix16_t x,fix16_t y,fix16_t w,fix16_t h,fix16_t xspd,fix16_t yspd);

#endif