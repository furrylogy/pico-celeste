#include "classic.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

extern type_t
    smoke,player,lifeup,fruit,orb,
    platform,room_title,player_spawn,
    spring,balloon,fall_floor,fruit,
    fly_fruit,fake_wall,key,chest,
    message,big_chest,flag;

room_t room={0,0};
size_t count_objects=0;
obj_t *objects[MAX_OBJECTS_COUNT]={NULL};
type_t *types[]=
{
    &player_spawn,&spring,&balloon,
    &fall_floor,&fruit,&fly_fruit,
    &fake_wall,&key,&chest,&message,
    &big_chest,&flag
};
int16_t freeze=0;
int16_t shake=0;
uint8_t will_restart=0;
int16_t delay_restart=0;
uint8_t got_fruit[30]={0};
uint8_t has_dashed=0;
int16_t sfx_timer=0;
uint8_t has_key=0;
uint8_t pause_player=0;
uint8_t flash_bg=0;
int16_t music_timer=0;
uint8_t new_bg=0;

int16_t frames;
int16_t seconds;
int16_t minutes;
int16_t deaths;
int16_t max_djump;
uint8_t start_game;
int16_t start_game_flash;

//entry point

void _init(void)
{
    clouds_init();
    particles_init();

    title_screen();
}

void title_screen()
{
    memset(got_fruit,0,30);
    frames=0;
    deaths=0;
    max_djump=1;
    start_game=0;
    start_game_flash=0;
    music(40,0,7);
   
    load_room(7,3);
}

void begin_game()
{
    frames=0;
    seconds=0;
    minutes=0;
    music_timer=0;
    start_game=0;
    start_game_flash=0;
    music(0,0,7);
    load_room(0,0);
}

uint16_t level_index()
{
    return room.x%8+room.y*8;
}

uint8_t is_title()
{
    return level_index()==31;
}

//effects

cloud_t clouds[17];
void clouds_init()
{
    for(int16_t i=0;i<=16;i++)
    {
        clouds[i].x=rnd(F16(128));
        clouds[i].y=rnd(F16(128));
        clouds[i].spd=fix16_add(F16(1),rnd(F16(4)));
        clouds[i].w=fix16_add(F16(32),rnd(F16(32)));
    }
}

particle_t particles[25];
void particles_init()
{
    for(int16_t i=0;i<=24;i++)
    {
        particles[i].x=rnd(F16(128));
        particles[i].y=rnd(F16(128));
        particles[i].s=fix16_add(F16(0),fix16_floor(fix16_div(rnd(F16(5)),F16(4)))); //0+flr(rnd(5)/4)
        particles[i].spd=fix16_add(F16(0.25),rnd(F16(5)));
        particles[i].off=rnd(F16(1));
        particles[i].c=6+fix16_to_int(fix16_floor(fix16_add(F16(0.5),rnd(F16(1))))); //6+flr(0.5+rnd(1))
    }
}

dead_particle_t dead_particles[8];

//player entity
void player_init(obj_t* obj)
{
    player_t* this = (player_t*)obj;
    this->p_jump=0;
    this->p_dash=0;
    this->grace=0;
    this->jbuffer=0;
    this->djump=max_djump;
    this->dash_time=0;
    this->dash_effect_time=0;
    this->dash_target=(vec2d_t){F16(0),F16(0)};
    this->dash_accel=(vec2d_t){F16(0),F16(0)};
    obj->hitbox=(box2d_t){F16(1),F16(3),F16(6),F16(5)};
    this->spr_off=0;
    this->was_on_ground=0;
    create_hair(obj);
}
void player_update(obj_t* obj)
{
    if (pause_player) return;
    int16_t input = btn(k_right) ? 1 : (btn(k_left) ? -1 : 0);

    player_t* this = (player_t*)obj;
    //spikes collide
    if(spikes_at(fix16_add(obj->x,obj->hitbox.x),fix16_add(obj->y,obj->hitbox.y),obj->hitbox.w,obj->hitbox.h,obj->spd.x,obj->spd.y))
    {
        kill_player(obj);
    }

    //bottom death
    if (obj->y > F16(128))
    {
        kill_player(obj);
    }

    uint8_t on_ground=obj->is_solid(obj,F16(0),F16(1));
    uint8_t on_ice=obj->is_ice(obj,F16(0),F16(1));

    //smoke particles
    if(on_ground && !(this->was_on_ground))
    {
        init_object(&smoke,obj->x,fix16_add(obj->y,F16(4)));
    }

    uint8_t jump = btn(k_jump) && !(this->p_jump);
    this->p_jump = btn(k_jump);
    if(jump)
    {
        this->jbuffer=4;
    }
    else if(this->jbuffer>0)
    {
        this->jbuffer-=1;
    }

    uint8_t dash = btn(k_dash) && !(this->p_dash);
    this->p_dash = btn(k_dash);

    if(on_ground)
    {
        this->grace=6;
        if(this->djump < max_djump)
        {
            psfx(54);
            this->djump=max_djump;
        }
    }
    else if(this->grace>0)
    {
        this->grace-=1;
    }

    this->dash_effect_time-=1;
    if(this->dash_time>0)
    {
        init_object(&smoke,obj->x,obj->y);
        this->dash_time-=1;
        obj->spd.x=appr(obj->spd.x,this->dash_target.x,this->dash_accel.x);
        obj->spd.y=appr(obj->spd.y,this->dash_target.y,this->dash_accel.y);
    }
    else
    {
        //move
        fix16_t maxrun=F16(1);
        fix16_t accel=F16(0.6);
        fix16_t deccel=F16(0.15);

        if(!on_ground)
        {
            accel=F16(0.4);
        }
        else if(on_ice)
        {
            accel=F16(0.05);
            if(input==(obj->flip.x?-1:1))
            {
                accel=F16(0.05);
            }
        }

        if(fix16_abs(obj->spd.x) > maxrun)
        {
            obj->spd.x=appr(obj->spd.x,fix16_mul(sign(obj->spd.x),maxrun),deccel);
        }
        else
        {
            obj->spd.x=appr(obj->spd.x,fix16_mul(fix16_from_int(input),maxrun),accel);
        } 

        //facing
        if(obj->spd.x != F16(0))
        {
            obj->flip.x=(obj->spd.x<F16(0));
        }

        //gravity
        fix16_t maxfall=F16(2);
        fix16_t gravity=F16(0.21);

        if(fix16_abs(obj->spd.y) <= F16(0.15))
        {
            gravity=fix16_mul(gravity,F16(0.5));
        }

        //wall slide
        if(input!=0 &&
            obj->is_solid(obj,fix16_from_int(input),F16(0)) &&
            !obj->is_ice(obj,fix16_from_int(input),F16(0))
        )
        {
            maxfall=F16(0.4);
            if(rnd(F16(10))<F16(2))
            {
                init_object(&smoke,fix16_add(obj->x,fix16_mul(fix16_from_int(input),F16(6))),obj->y);
            }
        }

        if(!on_ground)
        {
            obj->spd.y=appr(obj->spd.y,maxfall,gravity);
        }

        //jump
        if(this->jbuffer>0)
        {
            if(this->grace>0)
            {
                //normal jump
                psfx(1);
                this->jbuffer=0;
                this->grace=0;
                obj->spd.y=F16(-2);
                init_object(&smoke,obj->x,fix16_add(obj->y,F16(4)));
            }
            else
            {
                //wall jump
                fix16_t wall_dir=obj->is_solid(obj,F16(-3),F16(0))?F16(-1):obj->is_solid(obj,F16(3),F16(0))?F16(1):F16(0);
                if(wall_dir!=F16(0))
                {
                    psfx(2);
                    obj->spd.y=F16(-2);
                    obj->spd.x=fix16_sub(F16(0),fix16_mul(wall_dir,fix16_add(maxrun,F16(1))));
                    if(!obj->is_ice(obj,fix16_mul(wall_dir,F16(3)),F16(0)))
                    {
                        init_object(&smoke,fix16_add(obj->x,fix16_mul(wall_dir,F16(6))),obj->y);
                    }
                }
            }
        }

        //dash
        fix16_t d_full=F16(5);
        fix16_t d_half=fix16_mul(d_full,F16(0.70710678118));

        if(this->djump>0 && dash)
        {
            init_object(&smoke,obj->x,obj->y);
            this->djump-=1;
            this->dash_time=4;
            has_dashed=1;
            this->dash_effect_time=10;
            int16_t v_input = btn(k_up) ? -1 : (btn(k_down) ? 1 : 0);
            if(input!=0)
            {
                if(v_input!=0)
                {
                    obj->spd.x=fix16_mul(fix16_from_int(input),d_half);
                    obj->spd.y=fix16_mul(fix16_from_int(v_input),d_half);
                }
                else
                {
                    obj->spd.x=fix16_mul(fix16_from_int(input),d_full);
                    obj->spd.y=F16(0);
                }
            }
            else if(v_input!=0)
            {
                obj->spd.x=F16(0);
                obj->spd.y=fix16_mul(fix16_from_int(v_input),d_full);
            }
            else
            {
                obj->spd.x=obj->flip.x?F16(-1):F16(1);
                obj->spd.y=F16(0);
            }

            psfx(3);
            freeze=2;
            shake=6;
            this->dash_target.x=fix16_mul(F16(2),sign(obj->spd.x));
            this->dash_target.y=fix16_mul(F16(2),sign(obj->spd.y));
            this->dash_accel.x=F16(1.5);
            this->dash_accel.y=F16(1.5);

            if(obj->spd.y<F16(0))
            {
                this->dash_target.y=fix16_mul(this->dash_target.y,F16(0.75));
            }
            if(obj->spd.y!=F16(0))
            {
                this->dash_accel.x=fix16_mul(this->dash_accel.x,F16(0.70710678118));
            }
            if(obj->spd.x!=F16(0))
            {
                this->dash_accel.y=fix16_mul(this->dash_accel.y,F16(0.70710678118));
            }
        }
        else if(dash && this->djump<=0)
        {
            psfx(9);
            init_object(&smoke,obj->x,obj->y);
        }
    }
    //animation
    this->spr_off=(this->spr_off+1)%16;
    if(!on_ground)
    {
        if(obj->is_solid(obj,fix16_from_int(input),F16(0)))
        {
            obj->spr=5;
        }
        else
        {
            obj->spr=3;
        }
    }
    else if(btn(k_down))
    {
        obj->spr=6;
    }
    else if(btn(k_up))
    {
        obj->spr=7;
    }
    else if(obj->spd.x==F16(0) || (!btn(k_left) && !btn(k_right)))
    {
        obj->spr=1;
    }
    else
    {
        obj->spr=1+this->spr_off/4%4;
    }

    //next level
    if(obj->y<F16(-4) && level_index()<30)
    {
        next_room();
    }

    //was on the ground
    this->was_on_ground=on_ground;
}
void player_draw(obj_t* obj)
{
    player_t* this = (player_t*)obj;
    //clamp in screen
    if(obj->x<F16(-1) || obj->x>F16(121))
    {
        obj->x=clamp(obj->x,F16(-1),F16(121));
        obj->spd.x=0;
    }

    set_hair_color(this->djump);
    draw_hair(obj,obj->flip.x?F16(-1):F16(1));
    spr(obj->spr,obj->x,obj->y,obj->flip.x,obj->flip.y);
    unset_hair_color();
}
type_t player = {
    0,  //tile
    0,  //if_not_fruit
    sizeof(player_t),
    player_init,
    player_update,
    player_draw
};

void psfx(uint8_t num)
{
    if(sfx_timer<=0)
    {
        sfx(num);
    }
}
void create_hair(obj_t* obj)
{
    hair_t* hair = ((playerobj_t*)obj)->hair;
    for(int16_t i=0;i<=4;i++)
    {
        hair[i].x=obj->x;
        hair[i].y=obj->y;
        hair[i].size=clamp(fix16_from_int(3-i),F16(1),F16(2));
    }
}
void set_hair_color(int16_t djump)
{
    pal(8,djump==1?8:(djump==2?((frames/3&1)?11:7):12));
}
void draw_hair(obj_t* obj,fix16_t facing)
{
    hair_t *h = ((playerobj_t*)obj)->hair;
    hair_t last;
    last.x=fix16_sub(fix16_add(obj->x,F16(4)),fix16_mul(facing,F16(2)));
    last.y=fix16_add(obj->y,(btn(k_down)?F16(4):F16(3)));
    for(int16_t i=0;i<=4;i++)
    {
        h[i].x=fix16_add(h[i].x,fix16_div(fix16_sub(last.x,h[i].x),F16(1.5)));
        h[i].y=fix16_add(h[i].y,fix16_div(fix16_sub(fix16_add(last.y,F16(0.5)),h[i].y),F16(1.5)));
        circfill(h[i].x,h[i].y,h[i].size,8);
        last=h[i];
    }
}
void unset_hair_color()
{
    pal(8,8);
}

void player_spawn_init(obj_t* obj)
{
    player_spawn_t* this = (player_spawn_t*)obj;
    sfx(4);
    obj->spr=3;
    this->target=(vec2d_t){obj->x,obj->y};
    obj->y=F16(128);
    obj->spd.y=F16(-4);
    this->state=0;
    this->delay=0;
    obj->solids=0;
    create_hair(obj);
}
void player_spawn_update(obj_t* obj)
{
    player_spawn_t* this = (player_spawn_t*)obj;
    //jumping up
    if(this->state==0)
    {
        if(obj->y < fix16_add(this->target.y,F16(16)))
        {
            this->state=1;
            this->delay=3;
        }
    }
    //falling
    else if(this->state==1)
    {
        obj->spd.y=fix16_add(obj->spd.y,F16(0.5));
        if(obj->spd.y>F16(0) && this->delay>0)
        {
            obj->spd.y=F16(0);
            this->delay-=1;
        }
        if(obj->spd.y>F16(0) && obj->y>this->target.y)
        {
            obj->y=this->target.y;
            obj->spd = (vec2d_t){F16(0),F16(0)};
            this->state=2;
            this->delay=5;
            shake=5;
            init_object(&smoke,obj->x,fix16_add(obj->y,F16(4)));
            sfx(5);
        }
    }
    //landing
    else if(this->state==2)
    {
        this->delay-=1;
        obj->spr=6;
        if(this->delay<0){
            destroy_object(obj);
            init_object(&player,obj->x,obj->y);
        }
    }
}
void player_spawn_draw(obj_t* obj)
{
    set_hair_color(max_djump);
    draw_hair(obj,F16(1));
    spr(obj->spr,obj->x,obj->y,obj->flip.x,obj->flip.y);
    unset_hair_color();
}
type_t player_spawn = {
    1,
    0,
    sizeof(player_spawn_t),
    player_spawn_init,
    player_spawn_update,
    player_spawn_draw
};

void spring_init(obj_t* obj)
{
    spring_t* this = (spring_t*)obj;
    this->hide_in=0;
    this->hide_for=0;
}
void spring_update(obj_t* obj)
{
    spring_t* this = (spring_t*)obj;
    if(this->hide_for>0)
    {
        this->hide_for-=1;
        if(this->hide_for<=0)
        {
            obj->spr=18;
            this->delay=0;
        }
    }
    else if(obj->spr==18)
    {
        obj_t* hit = obj->collide(obj,&player,F16(0),F16(0));
        if(hit!=NULL && hit->spd.y>=F16(0))
        {
            obj->spr=19;
            hit->y=fix16_sub(hit->y,F16(4));
            hit->spd.x=fix16_mul(hit->spd.x,F16(0.2));
            hit->spd.y=F16(-3);
            ((player_t*)hit)->djump=max_djump;
            this->delay=10;
            init_object(&smoke,obj->x,obj->y);
            
            //breakable below us
            obj_t* below=obj->collide(obj,&fall_floor,F16(0),F16(1));
            if(below!=NULL)
            {
                break_fall_floor(below);
            }
            psfx(8);
        }
    }
    else if(this->delay>0)
    {
        this->delay-=1;
        if(this->delay<=0)
        {
            obj->spr=18;
        }
    }
    //begin hiding
    if(this->hide_in>0)
    {
        this->hide_in-=1;
        if(this->hide_in<=0)
        {
            this->hide_for=60;
            obj->spr=0;
        }
    }
}
type_t spring = {
    18,
    0,
    sizeof(spring_t),
    spring_init,
    spring_update,
    NULL
};

void break_spring(obj_t* obj)
{
    ((spring_t*)obj)->hide_in=15;
}

void balloon_init(obj_t* obj)
{
    balloon_t* this = (balloon_t*)obj;
    this->offset=rnd(F16(1));
    this->start=obj->y;
    this->timer=0;
    obj->hitbox=(box2d_t){F16(-1),F16(-1),F16(10),F16(10)};
}
void balloon_update(obj_t* obj)
{
    balloon_t* this = (balloon_t*)obj;
    if(obj->spr==22)
    {
        this->offset=fix16_add(this->offset,F16(0.01));
        obj->y=fix16_add(this->start,fix16_mul(p8sin(this->offset),F16(2)));
        obj_t* hit = obj->collide(obj,&player,F16(0),F16(0));
        if(hit!=NULL && ((player_t*)hit)->djump<max_djump)
        {
            psfx(6);
            init_object(&smoke,obj->x,obj->y);
            ((player_t*)hit)->djump=max_djump;
            obj->spr=0;
            this->timer=60;
        }
    }
    else if(this->timer>0)
    {
        this->timer-=1;
    }
    else
    {
        psfx(7);
        init_object(&smoke,obj->x,obj->y);
        obj->spr=22;
    }
}
void balloon_draw(obj_t* obj)
{
    balloon_t* this = (balloon_t*)obj;
    if(obj->spr==22)
    {
        spr(13+fix16_to_int(fix16_mod(fix16_mul(this->offset,F16(8)),F16(3))),obj->x,fix16_add(obj->y,F16(6)),0,0);
        spr(obj->spr,obj->x,obj->y,0,0);
    }
}
type_t balloon = {
    22,
    0,
    sizeof(balloon_t),
    balloon_init,
    balloon_update,
    balloon_draw
};

void fall_floor_init(obj_t* obj)
{
    fall_floor_t* this = (fall_floor_t*)obj;
    this->state=0;
    this->solid=1;
}
void fall_floor_update(obj_t* obj)
{
    fall_floor_t* this = (fall_floor_t*)obj;
    //idling
    if(this->state==0)
    {
        if(obj->check(obj,&player,F16(0),F16(-1)) || obj->check(obj,&player,F16(-1),F16(0)) || obj->check(obj,&player,F16(1),F16(0)))
        {
            break_fall_floor(obj);
        }
    }
    //shaking
    else if(this->state==1)
    {
        this->delay-=1;
        if(this->delay<=0)
        {
            this->state=2;
            this->delay=60;//how long it hides for
            obj->collideable=0;
        }
    }
    //invisible, waiting to reset
    else if (this->state==2)
    {
        this->delay-=1;
        if(this->delay<=0 && !obj->check(obj,&player,F16(0),F16(0)))
        {
            psfx(7);
            this->state=0;
            obj->collideable=1;
            init_object(&smoke,obj->x,obj->y);
        }
    }
}
void fall_floor_draw(obj_t* obj)
{
    fall_floor_t* this = (fall_floor_t*)obj;
    if(this->state!=2)
    {
        if(this->state!=1)
        {
            spr(23,obj->x,obj->y,0,0);
        }
        else
        {
            spr(23+(15-this->delay)/5,obj->x,obj->y,0,0);
        }
    }
}
type_t fall_floor = {
    23,
    0,
    sizeof(fall_floor_t),
    fall_floor_init,
    fall_floor_update,
    fall_floor_draw
};

void break_fall_floor(obj_t* obj)
{
    fall_floor_t* this = (fall_floor_t*)obj;
    if(this->state==0)
    {
        psfx(15);
        this->state=1;
        this->delay=15;//how long until it falls
        init_object(&smoke,obj->x,obj->y);
        obj_t* hit=obj->collide(obj,&spring,F16(0),F16(-1));
        if(hit!=NULL)
        {
            break_spring(hit);
        }
    }
}

void smoke_init(obj_t* obj)
{
    smoke_t* this = (smoke_t*)obj;
    this->spr_off=0;
    obj->spd.y=F16(-0.1);
    obj->spd.x=F16(0.3)+rnd(F16(0.2));
    obj->x=fix16_add(obj->x,fix16_add(F16(-1),rnd(F16(2))));
    obj->y=fix16_add(obj->y,fix16_add(F16(-1),rnd(F16(2))));
    obj->flip.x=maybe();
    obj->flip.y=maybe();
    obj->solids=0;
}
void smoke_update(obj_t* obj)
{
    smoke_t* this = (smoke_t*)obj;
    
    this->spr_off+=1;
    obj->spr=29+this->spr_off/5;
    if(obj->spr>=32)
    {
        destroy_object(obj);
    }
}
type_t smoke = {
    29,
    0,
    sizeof(smoke_t),
    smoke_init,
    smoke_update,
    NULL
};

void fruit_init(obj_t* obj)
{
    fruit_t* this = (fruit_t*)obj;
    this->start=obj->y;
    this->off=F16(0);
}
void fruit_update(obj_t* obj)
{
    fruit_t* this = (fruit_t*)obj;
    obj_t* hit = obj->collide(obj,&player,F16(0),F16(0));
    if(hit!=NULL)
    {
        ((player_t*)hit)->djump=max_djump;
        sfx_timer=20;
        sfx(13);
        got_fruit[level_index()]=1;
        init_object(&lifeup,obj->x,obj->y);
        destroy_object(obj);
    }
    this->off=fix16_add(this->off,F16(1));
    obj->y=fix16_add(this->start,fix16_mul(p8sin(fix16_div(this->off,F16(40))),F16(2.5)));
}
type_t fruit = {
    26,
    1,
    sizeof(fruit_t),
    fruit_init,
    fruit_update,
    NULL
};

void fly_fruit_init(obj_t* obj)
{
    fly_fruit_t* this = (fly_fruit_t*)obj;
    this->start=obj->y;
    this->fly=0;
    this->step=F16(0.5);
    obj->solids=0;
    this->sfx_delay=8;
}
void fly_fruit_update(obj_t* obj)
{
    fly_fruit_t* this = (fly_fruit_t*)obj;
    //fly away
    if(this->fly)
    {
        if(this->sfx_delay>0)
        {
            this->sfx_delay-=1;
            if(this->sfx_delay<=0)
            {
                sfx_timer=20;
                sfx(14);
            }
        }
        obj->spd.y=appr(obj->spd.y,F16(-3.5),F16(0.25));
        if(obj->y<F16(-16))
        {
            destroy_object(obj);
        }
    }
    //wait
    else
    {
        if(has_dashed)
        {
            this->fly=1;
        }
        this->step=fix16_add(this->step,F16(0.05));
        obj->spd.y=fix16_mul(p8sin(this->step),F16(0.5));

    }
    //collect
    obj_t* hit = obj->collide(obj,&player,F16(0),F16(0));
    if(hit!=NULL)
    {
        ((player_t*)hit)->djump=max_djump;
        sfx_timer=20;
        sfx(13);
        got_fruit[level_index()]=1;
        init_object(&lifeup,obj->x,obj->y);
        destroy_object(obj);
    }
}
void fly_fruit_draw(obj_t* obj)
{
    fly_fruit_t* this = (fly_fruit_t*)obj;
    fix16_t off=F16(0);
    if(!this->fly)
    {
        fix16_t dir=p8sin(this->step);
        if(dir<F16(0))
        {
            off=fix16_add(F16(1),fix16_max(F16(0),sign(fix16_sub(obj->y,this->start))));
        }
    }
    else
    {
        off=fix16_mod(fix16_add(off,F16(0.25)),F16(3));
    }
    spr(45+fix16_to_int(off),fix16_sub(obj->x,F16(6)),fix16_sub(obj->y,F16(2)),1,0);
    spr(obj->spr,obj->x,obj->y,0,0);
    spr(45+fix16_to_int(off),fix16_add(obj->x,F16(6)),fix16_sub(obj->y,F16(2)),0,0);
}
type_t fly_fruit = {
    28,
    1,
    sizeof(fly_fruit_t),
    fly_fruit_init,
    fly_fruit_update,
    fly_fruit_draw
};

void lifeup_init(obj_t* obj)
{
    lifeup_t* this = (lifeup_t*)obj;
    obj->spd.y=F16(-0.25);
    this->duration=30;
    obj->x=fix16_sub(obj->x,F16(2));
    obj->y=fix16_sub(obj->y,F16(4));
    this->flash=0;
    obj->solids=0;
    printf("lifeup init\n");
}
void lifeup_update(obj_t* obj)
{
    lifeup_t* this = (lifeup_t*)obj;
    this->duration-=1;
    if(this->duration<=0)
    {
        destroy_object(obj);
    }
}
void lifeup_draw(obj_t* obj)
{
    lifeup_t* this = (lifeup_t*)obj;
    this->flash+=1;
    print("1000",fix16_sub(obj->x,F16(2)),obj->y,7+this->flash/2%2);
}
type_t lifeup = {
    0,
    0,
    sizeof(lifeup_t),
    lifeup_init,
    lifeup_update,
    lifeup_draw
};

void fake_wall_update(obj_t* obj)
{
    obj->hitbox = (box2d_t){F16(-1), F16(-1), F16(18), F16(18)};
    obj_t* hit = obj->collide(obj,&player,F16(0),F16(0));
    if(hit!=NULL && ((player_t*)hit)->dash_effect_time>0)
    {
        hit->spd.x=fix16_mul(fix16_sub(F16(0),sign(hit->spd.x)),F16(1.5));
        hit->spd.y=F16(-1.5);
        ((player_t*)hit)->dash_time=-1;
        sfx_timer=20;
        sfx(16);
        destroy_object(obj);
        init_object(&smoke,obj->x,obj->y);
        init_object(&smoke,fix16_add(obj->x,F16(8)),obj->y);
        init_object(&smoke,obj->x,fix16_add(obj->y,F16(8)));
        init_object(&smoke,fix16_add(obj->x,F16(8)),fix16_add(obj->y,F16(8)));
        init_object(&fruit,fix16_add(obj->x,F16(4)),fix16_add(obj->y,F16(4)));
    }
    obj->hitbox=(box2d_t){F16(0),F16(0),F16(16),F16(16)};
}
void fake_wall_draw(obj_t* obj)
{
    spr(64,obj->x,obj->y,0,0);
    spr(65,fix16_add(obj->x,F16(8)),obj->y,0,0);
    spr(80,obj->x,fix16_add(obj->y,F16(8)),0,0);
    spr(81,fix16_add(obj->x,F16(8)),fix16_add(obj->y,F16(8)),0,0);
}
type_t fake_wall = {
    64,
    1,
    sizeof(fake_wall_t),
    NULL,
    fake_wall_update,
    fake_wall_draw
};

void key_init(obj_t* obj)
{
    key_t* this = (key_t*)obj;
    this->spr=fix16_from_int(obj->spr);
}
void key_update(obj_t* obj)
{
    key_t* this = (key_t*)obj;
    fix16_t was=fix16_floor(this->spr);
    this->spr=fix16_add(F16(9),fix16_mul(fix16_add(p8sin(fix16_div(fix16_from_int(frames),F16(30))),F16(0.5)),F16(1)));
    fix16_t is=fix16_floor(this->spr);
    obj->spr=fix16_to_int(this->spr);
    if(is==F16(10) && is!=was)
    {
        obj->flip.x =!obj->flip.x;
    }
    if(obj->check(obj,&player,F16(0),F16(0)))
    {
        sfx(23);
        sfx_timer=10;
        destroy_object(obj);
        has_key=1;
    }
}
type_t key = {
    8,
    1,
    sizeof(key_t),
    key_init,
    key_update,
    NULL
};

void chest_init(obj_t* obj)
{
    chest_t* this = (chest_t*)obj;
    obj->x=fix16_sub(obj->x,F16(4));
    this->start=obj->x;
    this->timer=20;
}
void chest_update(obj_t* obj)
{
    chest_t* this = (chest_t*)obj;
    if(has_key)
    {
        this->timer-=1;
        obj->x=fix16_add(fix16_sub(this->start,F16(1)),rnd(F16(3)));
        if(this->timer<=0)
        {
            sfx_timer=20;
            sfx(16);
            init_object(&fruit,obj->x,fix16_sub(obj->y,F16(4)));
            destroy_object(obj);
        }
    }
}
type_t chest = {
    20,
    1,
    sizeof(chest_t),
    chest_init,
    chest_update,
    NULL
};

void platform_init(obj_t* obj)
{
    platform_t* this = (platform_t*)obj;
    obj->x=fix16_sub(obj->x,F16(4));
    obj->solids=0;
    obj->hitbox.w=F16(16);
    this->last=obj->x;
}
void platform_update(obj_t* obj)
{
    platform_t* this = (platform_t*)obj;
    obj->spd.x=fix16_mul(this->dir,F16(0.65));
    if(obj->x<F16(-16))
    {
        obj->x=F16(128);
    }
    else if(obj->x>F16(128))
    {
        obj->x=F16(-16);
    }
    if(!obj->check(obj,&player,F16(0),F16(0)))
    {
        obj_t* hit = obj->collide(obj,&player,F16(0),F16(-1));
        if(hit!=NULL)
        {
            hit->move_x(hit,fix16_sub(obj->x,this->last),1);
        }
    }
    this->last=obj->x;
}
void platform_draw(obj_t* obj)
{
    spr(11,obj->x,fix16_sub(obj->y,F16(1)),0,0);
    spr(12,fix16_add(obj->x,F16(8)),fix16_sub(obj->y,F16(1)),0,0);
}
type_t platform = {
    0,
    0,
    sizeof(platform_t),
    platform_init,
    platform_update,
    platform_draw
};

void message_draw(obj_t* obj)
{
    message_t* this = (message_t*)obj;
    const char* text="-- celeste mountain --#this memorial to those# perished on the climb";
    if(obj->check(obj,&player,F16(4),F16(0)))
    {
        if(this->index/2<strlen(text))
        {
            this->index+=1;
            if(this->index/2>=this->last+1)
            {
                this->last+=1;
                sfx(35);
            }
        }
        vec2d_t off={F16(8),F16(96)};
        for(int16_t i=0;i<this->index/2;i++)
        {
            if(text[i]!='#')
            {
                rectfill(fix16_sub(off.x,F16(2)),fix16_sub(off.y,F16(2)),fix16_add(off.x,F16(7)),fix16_add(off.y,F16(6)),7);
                print("%c",off.x,off.y,0,text[i]);
                off.x=fix16_add(off.x,F16(5));
            }
            else
            {
                off.x=F16(8);
                off.y=fix16_add(off.y,F16(7));
            }
        }
    }
    else
    {
        this->index=0;
        this->last=0;
    }
}
type_t message = {
    86,
    0,
    sizeof(message_t),
    NULL,
    NULL,
    message_draw
};

void big_chest_init(obj_t* obj)
{
    big_chest_t* this = (big_chest_t*)obj;
    this->state=0;
    obj->hitbox.w=F16(16);
}
void big_chest_draw(obj_t* obj)
{
    big_chest_t* this = (big_chest_t*)obj;
    if(this->state==0)
    {
        obj_t* hit = obj->collide(obj,&player,F16(0),F16(8));
        if(hit!=NULL && hit->is_solid(hit,F16(0),F16(1)))
        {
            music(-1,500,7);
            sfx(37);
            pause_player=1;
            hit->spd.x=F16(0);
            hit->spd.y=F16(0);
            this->state=1;
            init_object(&smoke,obj->x,obj->y);
            init_object(&smoke,fix16_add(obj->x,F16(8)),obj->y);
            this->timer=60;
            memset(this->particles,0,sizeof(this->particles));
            this->particle_count=0;
        }
        spr(96,obj->x,obj->y,0,0);
        spr(97,fix16_add(obj->x,F16(8)),obj->y,0,0);
    }
    else if(this->state==1)
    {
        this->timer-=1;
        shake=5;
        flash_bg=1;
        if(this->timer<=45 && this->particle_count<50)
        {
            this->particles[this->particle_count++]=
            (bc_particle_t){
                fix16_add(F16(1),rnd(F16(14))),
                F16(0),
                fix16_add(F16(32),rnd(F16(32))),
                fix16_add(F16(8),rnd(F16(8)))
            };
        }
        if(this->timer<0)
        {
            this->state=2;
            memset(this->particles,0,sizeof(this->particles));
            this->particle_count=0;
            flash_bg=0;
            new_bg=1;
            init_object(&orb,fix16_add(obj->x,F16(4)),fix16_add(obj->y,F16(4)));
            pause_player=0;
        }
        for(int16_t i=0;i<this->particle_count;i++)
        {
            bc_particle_t* p = &this->particles[i];
            p->y=fix16_add(p->y,p->spd);
            rectfill(fix16_add(obj->x,p->x),fix16_sub(fix16_add(obj->y,F16(8)),p->y),fix16_add(obj->x,p->x),fix16_min(fix16_add(fix16_sub(fix16_add(obj->y,F16(8)),p->y),p->h),fix16_add(obj->y,F16(8))),7);
        }
    }
    spr(112,obj->x,fix16_add(obj->y,F16(8)),0,0);
    spr(113,fix16_add(obj->x,F16(8)),fix16_add(obj->y,F16(8)),0,0);
}
type_t big_chest = {
    96,
    0,
    sizeof(big_chest_t),
    big_chest_init,
    NULL,
    big_chest_draw
};

void orb_init(obj_t* obj)
{
    obj->spd.y=F16(-4);
    obj->solids=0;
}
void orb_draw(obj_t* obj)
{
    obj->spd.y=appr(obj->spd.y,F16(0),F16(0.5));
    obj_t* hit = obj->collide(obj,&player,F16(0),F16(0));
    if(obj->spd.y==0 && hit!=NULL)
    {
        music_timer=45;
        sfx(51);
        freeze=10;
        shake=10;
        destroy_object(obj);
        max_djump=2;
        ((player_t*)hit)->djump=2;
    }
    spr(102,obj->x,obj->y,0,0);
    fix16_t off=fix16_div(fix16_from_int(frames),F16(30));
    for(fix16_t i=F16(0);i<=F16(7);i=fix16_add(i,F16(1)))
    {
        circfill(fix16_add(fix16_add(obj->x,F16(4)),fix16_mul(p8cos(fix16_add(off,fix16_div(i,F16(8)))),F16(8))),fix16_add(fix16_add(obj->y,F16(4)),fix16_mul(p8sin(fix16_add(off,fix16_div(i,F16(8)))),F16(8))),F16(1),7);
    }
} 
type_t orb = {
    0,
    0,
    sizeof(orb_t),
    orb_init,
    NULL,
    orb_draw
};

void flag_init(obj_t* obj)
{
    flag_t* this = (flag_t*)obj;
    obj->x=fix16_add(obj->x,F16(5));
    this->score=0;
    this->show=0;
    for(int16_t i=0;i<(sizeof(got_fruit)/sizeof(uint8_t));i++)
    {
        if(got_fruit[i])
        {
            this->score+=1;
        }
    }
}
void flag_draw(obj_t* obj)
{
    flag_t* this = (flag_t*)obj;
    obj->spr=118+(frames/5)%3;
    if(this->show)
    {
        rectfill(F16(32),F16(2),F16(96),F16(31),0);
        spr(26,F16(55),F16(6),0,0);
        print("x%u",F16(64),F16(9),7,this->score);
        draw_time(F16(49),F16(16));
        print("deaths:%u",F16(48),F16(24),7,deaths);
    }
    else if(obj->check(obj,&player,F16(0),F16(0)))
    {
        sfx(55);
        sfx_timer=30;
        this->show=1;
    }
}
type_t flag = {
    118,
    0,
    sizeof(flag_t),
    flag_init,
    NULL,
    flag_draw
};

void room_title_init(obj_t* obj)
{
    room_title_t* this = (room_title_t*)obj;
    this->delay=5;
}
void room_title_draw(obj_t* obj)
{
    room_title_t* this = (room_title_t*)obj;
    this->delay-=1;
    if(this->delay<-30)
    {
        destroy_object(obj);
    }
    else if(this->delay<0)
    {
        rectfill(F16(24),F16(58),F16(104),F16(70),0);
        if(room.x==3 && room.y==1)
        {
            print("old site",F16(48),F16(62),7);
        }
        else if(level_index()==30)
        {
            print("summit",F16(52),F16(62),7);
        }
        else
        {
            int16_t level=(1+level_index())*100;
            print("%u m",fix16_add(F16(52),(level<1000)?F16(2):F16(0)),F16(62),7,level);
        }
        draw_time(F16(4),F16(4));
    }
}
type_t room_title = {
    0,
    0,
    sizeof(room_title_t),
    room_title_init,
    NULL,
    room_title_draw
};

//object functions
uint8_t obj_is_solid(obj_t* obj,fix16_t ox,fix16_t oy)
{
    if(oy>F16(0) && !obj->check(obj,&platform,ox,F16(0)) && obj->check(obj,&platform,ox,oy))
    {
        return 1;
    }
    return solid_at(fix16_add(fix16_add(obj->x,obj->hitbox.x),ox),fix16_add(fix16_add(obj->y,obj->hitbox.y),oy),obj->hitbox.w,obj->hitbox.h)
        || obj->check(obj,&fall_floor,ox,oy)
        || obj->check(obj,&fake_wall,ox,oy);
}
uint8_t obj_is_ice(obj_t* obj,fix16_t ox,fix16_t oy)
{
    return ice_at(fix16_add(fix16_add(obj->x,obj->hitbox.x),ox),fix16_add(fix16_add(obj->y,obj->hitbox.y),oy),obj->hitbox.w,obj->hitbox.h);
}
obj_t* obj_collide(obj_t* obj,type_t* type,fix16_t ox,fix16_t oy)
{
    obj_t* other;
    for(int16_t i=0;i<count_objects;i++)
    {
        other = objects[i];
        if(other != NULL && other->valid && other->type==type && other!=obj && other->collideable &&
            fix16_add(fix16_add(other->x,other->hitbox.x),other->hitbox.w)>fix16_add(fix16_add(obj->x,obj->hitbox.x),ox) &&
            fix16_add(fix16_add(other->y,other->hitbox.y),other->hitbox.h)>fix16_add(fix16_add(obj->y,obj->hitbox.y),oy) &&
            fix16_add(other->x,other->hitbox.x)<fix16_add(fix16_add(fix16_add(obj->x,obj->hitbox.x),obj->hitbox.w),ox) &&
            fix16_add(other->y,other->hitbox.y)<fix16_add(fix16_add(fix16_add(obj->y,obj->hitbox.y),obj->hitbox.h),oy)
        )
        {
            return other;
        }
    }
    return NULL;
}
uint8_t obj_check(obj_t* obj,type_t* type,fix16_t ox,fix16_t oy)
{
    return obj->collide(obj,type,ox,oy)!=NULL;
}
void obj_move(obj_t* obj,fix16_t ox,fix16_t oy)
{
    fix16_t amount;
    //[x] get move amount
    obj->rem.x=fix16_add(obj->rem.x,ox);
    amount=fix16_floor(fix16_add(obj->rem.x,F16(0.5)));
    obj->rem.x=fix16_sub(obj->rem.x,amount);
    obj->move_x(obj,amount,0);

    //[y] get move amount
    obj->rem.y=fix16_add(obj->rem.y,oy);
    amount=fix16_floor(fix16_add(obj->rem.y,F16(0.5)));
    obj->rem.y=fix16_sub(obj->rem.y,amount);
    obj->move_y(obj,amount);
}
void obj_move_x(obj_t* obj,fix16_t amount,int16_t start)
{
    if(obj->solids)
    {
        fix16_t step = sign(amount);
        for(int16_t i=start;i<=fix16_to_int(fix16_abs(amount));i++)
        {
            if(!obj->is_solid(obj,step,F16(0)))
            {
                obj->x=fix16_add(obj->x,step);
            }
            else
            {
                obj->spd.x=F16(0);
                obj->rem.x=F16(0);
                break;
            }
        }
    }
    else
    {
        obj->x=fix16_add(obj->x,amount);
    }
}
void obj_move_y(obj_t* obj,fix16_t amount)
{
    if(obj->solids)
    {
        fix16_t step = sign(amount);
        for(int16_t i=0;i<=fix16_to_int(fix16_abs(amount));i++)
        {
            if(!obj->is_solid(obj,F16(0),step))
            {
                obj->y=fix16_add(obj->y,step);
            }
            else
            {
                obj->spd.y=F16(0);
                obj->rem.y=F16(0);
                break;
            }
        }
    }
    else
    {
        obj->y=fix16_add(obj->y,amount);
    }
}
obj_t* init_object(type_t* type,fix16_t x,fix16_t y)
{
    if(type->if_not_fruit==1 && got_fruit[level_index()])
    {
        return NULL;
    }
    obj_t* obj = malloc(type->size);
    if(obj==NULL)
    {
        printf("Memory allocation failed for %zu bytes\n", type->size);
        return NULL;
    }
    memset(obj,0,type->size);

    obj->type = type;
    obj->collideable=1;
    obj->solids=1;

    obj->spr = type->tile;
    obj->flip =(flip_t){0,0};

    obj->x = x;
    obj->y = y;
    obj->hitbox = (box2d_t){F16(0),F16(0),F16(8),F16(8)};

    obj->spd = (vec2d_t){F16(0),F16(0)};
    obj->rem = (vec2d_t){F16(0),F16(0)};

    obj->is_solid=obj_is_solid;
    obj->is_ice=obj_is_ice;
    obj->collide=obj_collide;
    obj->check=obj_check;
    obj->move=obj_move;
    obj->move_x=obj_move_x;
    obj->move_y=obj_move_y;

    if(count_objects<MAX_OBJECTS_COUNT)
    {
        obj->valid=1;
        objects[count_objects++]=obj;
        if(obj->type->init!=NULL)
        {
            obj->type->init(obj);
        }
    }
    else
    {
        free(obj);
        printf("Maximum object count %d reached. Object discarded.\n",MAX_OBJECTS_COUNT);
        return NULL;
    }
    return obj;
}
void destroy_object(obj_t* obj)
{
    obj->valid=0;
}
void kill_player(obj_t* obj)
{
    sfx_timer=12;
    sfx(0);
    deaths+=1;
    shake=10;
    destroy_object(obj);
    for(int16_t dir=0;dir<(sizeof(dead_particles)/sizeof(dead_particle_t));dir++)
    {
        fix16_t angle=fix16_div(fix16_from_int(dir),F16(8));
        dead_particle_t* p = &dead_particles[dir];
        p->alive=1;
        p->x=fix16_add(obj->x,F16(4));
        p->y=fix16_add(obj->y,F16(4));
        p->t=F16(10);
        p->spd.x=fix16_mul(p8sin(angle),F16(3));                
        p->spd.y=fix16_mul(p8cos(angle),F16(3));
    }
    restart_room();
}

//room functions
void restart_room()
{
    will_restart=1;
    delay_restart=15;
}
void next_room()
{
    if(room.x==2 && room.y==1)
    {
        music(30,500,7);
    }
    else if(room.x==3 && room.y==1)
    {
        music(20,500,7);
    }
    else if(room.x==4 && room.y==2)
    {
        music(30,500,7);
    }
    else if(room.x==5 && room.y==3)
    {
        music(20,500,7);
    }
    if(room.x==7)
    {
        load_room(0,room.y+1);
    }
    else
    {
        load_room(room.x+1,room.y);
    }
}
void load_room(int16_t x,int16_t y)
{
    has_dashed=0;
    has_key=0;

    //remove existing objects
    for(int16_t i=0;i<count_objects;i++)
    {
        free(objects[i]);
        objects[i]=NULL;
    }
    count_objects=0;

    //current room
    room.x = x;
    room.y = y;

    //entities
    for(int16_t tx=0;tx<=15;tx++)
    {
        for(int16_t ty=0;ty<=15;ty++)
        {
            uint8_t tile = mget(room.x*16+tx,room.y*16+ty);
            platform_t* plt=NULL;
            if(tile==11)
            {
                plt=(platform_t*)init_object(&platform,fix16_from_int(tx*8),fix16_from_int(ty*8));
                if(plt!=NULL)plt->dir=F16(-1);
            }
            else if(tile==12)
            {
                plt=(platform_t*)init_object(&platform,fix16_from_int(tx*8),fix16_from_int(ty*8));
                if(plt!=NULL)plt->dir=F16(1);
            }
            else
            {
                for(int16_t i=0;i<(sizeof(types)/sizeof(type_t*));i++)
                {
                    if(types[i]->tile==tile)
                    {
                        init_object(types[i],fix16_from_int(tx*8),fix16_from_int(ty*8));
                    }
                }
            }
        }
    }
    if(!is_title())
    {
        init_object(&room_title,F16(0),F16(0));
    }
}

//update function
void _update()
{
    frames=((frames+1)%30);
    if(frames==0 && level_index()<30)
    {
        seconds=((seconds+1)%60);
        if(seconds==0)
        {
            minutes+=1;
        }
    }

    if(music_timer>0)
    {
        music_timer-=1;
        if(music_timer<=0)
        {
            music(10,0,7);
        }
    }

    if(sfx_timer>0)
    {
        sfx_timer-=1;
    }

    //cancel if freeze
    if(freeze>0)
    {
        freeze-=1;
        return;
    }

    //screenshake
    if(shake>0)
    {
        shake-=1;
        camera(F16(0),F16(0));
        if(shake>0)
        {
            camera(fix16_add(F16(-2),rnd(F16(5))),fix16_add(F16(-2),rnd(F16(5))));
        }
    }

    //restart (soon)
    if(will_restart && delay_restart>0)
    {
        delay_restart-=1;
        if(delay_restart<=0)
        {
            will_restart=0;
            load_room(room.x,room.y);
        }
    }

    //update each object
    int16_t new_count=0;
    for(int16_t i=0;i<count_objects;i++)
    {
        obj_t* obj=objects[i];
        if(obj!=NULL)
        {
            if(obj->valid)
            {
                obj->move(obj,obj->spd.x,obj->spd.y);
                if(obj->type->update!=NULL)
                {
                    obj->type->update(obj);
                }
                if(new_count<i)
                {
                    objects[new_count]=objects[i];
                    objects[i]=NULL;
                }
                new_count++;
            }
            else
            {
                free(obj);
                objects[i]=NULL;
            }
        }
    }
    count_objects=new_count;

    //start game
    if(is_title())
    {
        if(!start_game && (btn(k_jump) || btn(k_dash)))
        {
            music(-1,0,7);
            start_game_flash=50;
            start_game=1;
            sfx(38);
        }
        if(start_game)
        {
            start_game_flash-=1;
            if(start_game_flash<=-30)
            {
                begin_game();
            }
        }
    }
}

//drawing functions
void _draw()
{
    if(freeze>0) return;
    
    //reset all palette values
    for(int16_t i=0;i<16;i++)pal(i,i);

    //start game flash
    if(start_game)
    {
        uint8_t c=10;
        if(start_game_flash>10)
        {
            if(frames%10<5)
            {
                c=7;
            }
        }
        else if(start_game_flash>5)
        {
            c=2;
        }
        else if(start_game_flash>0)
        {
            c=0;
        }
        if(c<10)
        {
            pal(6,c);
            pal(12,c);
            pal(13,c);
            pal(5,c);
            pal(1,c);
            pal(7,c);
        }
    }

    //clear screen
    uint8_t bg_col=0;
    if(flash_bg)
    {
        bg_col=frames/5;
    }
    else if(new_bg)
    {
        bg_col=2;
    }
    rectfill(F16(0),F16(0),F16(128),F16(128),bg_col);

    //clouds
    if(!is_title())
    {
        for(int16_t i=0;i<(sizeof(clouds)/sizeof(cloud_t));i++)
        {
            cloud_t* c=&clouds[i];
            c->x=fix16_add(c->x,c->spd);
            rectfill(c->x,c->y,fix16_add(c->x,c->w),fix16_add(fix16_add(c->y,F16(4)),fix16_mul(fix16_sub(F16(1),fix16_div(c->w,F16(64))),F16(12))),new_bg?14:1);
            if(c->x>F16(128))
            {
                c->x=fix16_sub(F16(0),c->w);
                c->y=rnd(F16(128-8));
            }
        }
    }
    //draw bg terrain
    map(room.x * 16,room.y * 16,0,0,16,16,2);

    //platforms/big chest
    for(int16_t i=0;i<count_objects;i++)
    {
        obj_t* o=objects[i];
        if(o!=NULL && o->valid)
        {
            if(o->type==&platform || o->type==&big_chest)
            {
                draw_object(o);
            }
        }
    }

    //draw terrain
    map(room.x*16,room.y*16,(is_title()?-4:0),0,16,16,1);

    //draw objects
    for(int16_t i=0;i<count_objects;i++)
    {
        obj_t* o=objects[i];
        if(o!=NULL && o->valid)
        {
            if(o->type!=&platform && o->type!=&big_chest)
            {
                draw_object(o);
            }
        }
    }

    //draw fg terrain
    map(room.x * 16,room.y * 16,0,0,16,16,3);

    //particles
    for(int16_t i=0;i<(sizeof(particles)/sizeof(particle_t));i++)
    {
        particle_t* p = &particles[i];
        p->x=fix16_add(p->x,p->spd);
        p->y=fix16_add(p->y,p8sin(p->off));
        p->off=fix16_add(p->off,fix16_min(F16(0.05),fix16_div(p->spd,F16(32))));
        rectfill(p->x,p->y,fix16_add(p->x,p->s),fix16_add(p->y,p->s),p->c);
        if(p->x>F16(128+4))
        {
            p->x=F16(-4);
            p->y=rnd(F16(128));
        }
    }

    //dead particles
    for(int16_t i=0;i<(sizeof(dead_particles)/sizeof(dead_particle_t));i++)
    {
        dead_particle_t* p = &dead_particles[i];
        if(p->alive)
        {
            p->x=fix16_add(p->x,p->spd.x);
            p->y=fix16_add(p->x,p->spd.y);
            p->t=fix16_sub(p->t,F16(1));
            if(p->t<=F16(0))
            {
                p->alive=0;
            }
            rectfill(fix16_sub(p->x,fix16_div(p->t,F16(5))),fix16_sub(p->y,fix16_div(p->t,F16(5))),fix16_add(p->x,fix16_div(p->t,F16(5))),fix16_add(p->y,fix16_div(p->t,F16(5))),14+fix16_to_int(p->t)%2);
        }
    }

    //draw outside of the screen for screenshake
    rectfill(F16(-5),F16(-5),F16(-1),F16(133),0);
    rectfill(F16(-5),F16(-5),F16(133),F16(-1),0);
    rectfill(F16(-5),F16(128),F16(133),F16(133),0);
    rectfill(F16(128),F16(-5),F16(133),F16(133),0);

    //credits
    if(is_title())
    {
        print("x+c",F16(58),F16(80),5);
        print("matt thorson",F16(42),F16(96),5);
        print("noel berry",F16(46),F16(102),5);
    }

    if(level_index()==30)
    {
        obj_t* p=NULL;
        for(int16_t i=0;i<count_objects;i++)
        {
            p=NULL;
            if(objects[i]!=NULL&&objects[i]->valid && objects[i]->type==&player)
            {
                p=objects[i];
                break;
            }
        }
        if(p!=NULL)
        {
            fix16_t diff=fix16_min(F16(24),fix16_sub(F16(40),fix16_abs(fix16_add(p->x,F16(4-64)))));
            rectfill(F16(0),F16(0),diff,F16(128),0);
            rectfill(fix16_sub(F16(128),diff),F16(0),F16(128),F16(128),0);
        }
    }
}
void draw_object(obj_t* obj)
{
    if(obj->type->draw!=NULL)
    {
        obj->type->draw(obj);
    }
    else if(obj->spr>0)
    {
        spr(obj->spr,obj->x,obj->y,obj->flip.x,obj->flip.y);
    }
}
void draw_time(fix16_t x,fix16_t y)
{
    int16_t s=seconds;
    int16_t m=minutes%60;
    int16_t h=minutes/60;

    rectfill(x,y,fix16_add(x,F16(32)),fix16_add(y,F16(6)),0);
    print("%02d:%02d:%02d",fix16_add(x,F16(1)),fix16_add(y,F16(1)),7,h,m,s);
}

//helper functions
fix16_t clamp(fix16_t val,fix16_t a,fix16_t b)
{
    return fix16_max(a,fix16_min(b,val));
}
fix16_t appr(fix16_t val,fix16_t target,fix16_t amount)
{
    return val>target?
        fix16_max(fix16_sub(val,amount),target):
        fix16_min(fix16_add(val,amount),target);
}
fix16_t sign(fix16_t v)
{
    if (v > F16(0))
        return F16(1);
    if (v < F16(0))
        return F16(-1);
    return F16(0);
}
uint8_t maybe()
{
    return rnd(F16(1))<F16(0.5);
}

fix16_t p8sin(fix16_t x)
{
    return fix16_sub(F16(0),fix16_sin(fix16_mul(x,F16(6.283185307))));
}
fix16_t p8cos(fix16_t x)
{
    return fix16_cos(fix16_mul(x,F16(6.283185307)));
}

uint8_t solid_at(fix16_t x,fix16_t y,fix16_t w,fix16_t h)
{
    return tile_flag_at(x,y,w,h,0);
}
uint8_t ice_at(fix16_t x,fix16_t y,fix16_t w,fix16_t h)
{
    return tile_flag_at(x,y,w,h,4);
}
uint8_t tile_flag_at(fix16_t x,fix16_t y,fix16_t w,fix16_t h,uint8_t flag)
{
    for(
        int16_t i=fix16_to_int(fix16_max(F16(0),fix16_floor(fix16_div(x,F16(8))))), //max(0,flr(x/8))
            i_end=fix16_to_int(fix16_floor(fix16_min(F16(15),fix16_div(fix16_sub(fix16_add(x,w),F16(1)),F16(8))))),//flr(min(15,(x+w-1)/8))
            j_start=fix16_to_int(fix16_max(F16(0),fix16_floor(fix16_div(y,F16(8))))), //max(0,flr(y/8))
            j_end=fix16_to_int(fix16_floor(fix16_min(F16(15),fix16_div(fix16_sub(fix16_add(y,h),F16(1)),F16(8)))));//flr(min(15,(y+h-1)/8))
        i<=i_end;i++
    )
    {
        for(int16_t j=j_start;j<=j_end;j++)
        {
            if(fget(tile_at(i,j),flag))
            {
                return 1;
            }
        }
    }
    return 0;
}
uint8_t tile_at(int16_t x,int16_t y){
    return mget(room.x * 16 + x, room.y * 16 + y);
}
uint8_t spikes_at(fix16_t x,fix16_t y,fix16_t w,fix16_t h,fix16_t xspd,fix16_t yspd)
{
    for(
        int16_t i=fix16_to_int(fix16_max(F16(0),fix16_floor(fix16_div(x,F16(8))))), //max(0,flr(x/8))
            i_end=fix16_to_int(fix16_floor(fix16_min(F16(15),fix16_div(fix16_sub(fix16_add(x,w),F16(1)),F16(8))))),//flr(min(15,(x+w-1)/8))
            j_start=fix16_to_int(fix16_max(F16(0),fix16_floor(fix16_div(y,F16(8))))), //max(0,flr(y/8))
            j_end=fix16_to_int(fix16_floor(fix16_min(F16(15),fix16_div(fix16_sub(fix16_add(y,h),F16(1)),F16(8)))));//flr(min(15,(y+h-1)/8))
        i<=i_end;i++
    )
    {
        for(int16_t j=j_start;j<=j_end;j++)
        {
            uint8_t tile=tile_at(i,j);
            if((tile==17) && (
                    (fix16_mod(fix16_sub(fix16_add(y,h),F16(1)),F16(8)) >= F16(6)) ||
                    (fix16_to_int(fix16_add(y,h))==j*8+8)
                ) && (yspd >= F16(0))
            )
            {
                return 1;
            }
            else if((tile==27) && fix16_mod(y,F16(8))<=F16(2) && yspd<=F16(0))
            {
                return 1;
            }
            else if((tile==43) && fix16_mod(x,F16(8))<=F16(2) && xspd<=F16(0))
            {
                return 1;
            }
            else if((tile==59) && (
                    (fix16_mod(fix16_sub(fix16_add(x,w),F16(1)),F16(8)) >= F16(6)) ||
                    (fix16_to_int(fix16_add(x,w))==i*8+8)
                ) && (xspd >= F16(0))
            )
            {
                return 1;
            }
        }
    }
    return 0;
}