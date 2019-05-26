/*
==============================================================================

Mr. Zurkon

==============================================================================
*/

#include "m_zurkon.h"
#include "g_local.h"


static int	sound_pain;
static int	sound_pain2;
static int	sound_death;
static int	sound_idle;
static int	sound_open;
static int	sound_search;
static int	sound_sight;


void zurkon_idlesound(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void zurkon_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void zurkon_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}


qboolean visible(edict_t *self, edict_t *other);
//void ZurkonGrenade(edict_t *self);
void ZurkonFire(edict_t *self);
void zurkon_fire_chain(edict_t *self);
void zurkon_refire_chain(edict_t *self);


void zurkon_stand(edict_t *self);

mframe_t zurkon_frames_fidget[] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, zurkon_idlesound,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL
};
mmove_t	zurkon_move_fidget = { FRAME_stand31, FRAME_stand70, zurkon_frames_fidget, zurkon_stand };

void zurkon_fidget(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		return;
	if (random() <= 0.05)
		self->monsterinfo.currentmove = &zurkon_move_fidget;
}

mframe_t zurkon_frames_stand[] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, zurkon_fidget,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, zurkon_fidget,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, zurkon_fidget
};
mmove_t	zurkon_move_stand = { FRAME_stand01, FRAME_stand30, zurkon_frames_stand, NULL };

void zurkon_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &zurkon_move_stand;
}


mframe_t zurkon_frames_walk[] =
{
	ai_walk, 0, NULL,
	ai_walk, 3, NULL,
	ai_walk, 4, NULL,
	ai_walk, 5, NULL,
	ai_walk, 7, NULL,
	ai_walk, 2, NULL,
	ai_walk, 6, NULL,
	ai_walk, 4, NULL,
	ai_walk, 2, NULL,
	ai_walk, 7, NULL,
	ai_walk, 5, NULL,
	ai_walk, 7, NULL,
	ai_walk, 4, NULL
};
mmove_t zurkon_move_walk = { FRAME_walk07, FRAME_walk19, zurkon_frames_walk, NULL };

void zurkon_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &zurkon_move_walk;
}

mframe_t zurkon_frames_run[] =
{
	ai_run, 26, NULL,
	ai_run, 9, NULL,
	ai_run, 9, NULL,
	ai_run, 9, NULL,
	ai_run, 15, NULL,
	ai_run, 10, NULL,
	ai_run, 13, NULL,
	ai_run, 6, NULL
};

mmove_t zurkon_move_run = { FRAME_run01, FRAME_run08, zurkon_frames_run, NULL };

void zurkon_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &zurkon_move_stand;
	else
		self->monsterinfo.currentmove = &zurkon_move_run;
}

mframe_t zurkon_frames_runandshoot[] =
{
	ai_run, 32, NULL,
	ai_run, 15, NULL,
	ai_run, 10, NULL,
	ai_run, 18, NULL,
	ai_run, 8, NULL,
	ai_run, 20, NULL
};

mmove_t zurkon_move_runandshoot = { FRAME_runs01, FRAME_runs06, zurkon_frames_runandshoot, NULL };

void zurkon_runandshoot(edict_t *self)
{
	self->monsterinfo.currentmove = &zurkon_move_runandshoot;
}

mframe_t zurkon_frames_pain3[] =
{
	ai_move, -3, NULL,
	ai_move, 1, NULL,
	ai_move, 1, NULL,
	ai_move, 0, NULL,
	ai_move, 1, NULL
};
mmove_t zurkon_move_pain3 = { FRAME_pain301, FRAME_pain305, zurkon_frames_pain3, zurkon_run };

mframe_t zurkon_frames_pain2[] =
{
	ai_move, -2, NULL,
	ai_move, 11, NULL,
	ai_move, 6, NULL,
	ai_move, 2, NULL,
	ai_move, -1, NULL,
	ai_move, -7, NULL,
	ai_move, -2, NULL,
	ai_move, -7, NULL
};
mmove_t zurkon_move_pain2 = { FRAME_pain201, FRAME_pain208, zurkon_frames_pain2, zurkon_run };

mframe_t zurkon_frames_pain1[] =
{
	ai_move, 2, NULL,
	ai_move, 0, NULL,
	ai_move, -5, NULL,
	ai_move, 3, NULL,
	ai_move, -1, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 1, NULL,
	ai_move, 1, NULL,
	ai_move, 2, NULL,
	ai_move, 1, NULL,
	ai_move, 0, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t zurkon_move_pain1 = { FRAME_pain101, FRAME_pain118, zurkon_frames_pain1, zurkon_run };

void zurkon_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;

	if (rand() & 1)
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	if (skill->value == 3)
		return;		// no pain anims in nightmare

	if (damage <= 10)
		self->monsterinfo.currentmove = &zurkon_move_pain3;
	else if (damage <= 25)
		self->monsterinfo.currentmove = &zurkon_move_pain2;
	else
		self->monsterinfo.currentmove = &zurkon_move_pain1;
}

void zurkon_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

mframe_t zurkon_frames_death[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, -7, NULL,
	ai_move, -3, NULL,
	ai_move, -5, NULL,
	ai_move, 8, NULL,
	ai_move, 6, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t zurkon_move_death = { FRAME_death01, FRAME_death11, zurkon_frames_death, zurkon_dead };

void zurkon_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;
	// check for gib
	
	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n = 0; n < 2; n++)
			ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n = 0; n < 4; n++)
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		return;
	}
	
	if (self->deadflag == DEAD_DEAD)
		return;

	// regular death
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &zurkon_move_death;
	if (self->teammaster)
		self->teammaster->zurkon = 0;
}


void zurkon_duck_down(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;
	self->monsterinfo.aiflags |= AI_DUCKED;
	if (skill->value >= 2)
	{
		if (random() > 0.5)
			GunnerGrenade(self);
	}

	self->maxs[2] -= 32;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.pausetime = level.time + 1;
	gi.linkentity(self);
}

void zurkon_duck_hold(edict_t *self)
{
	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

void zurkon_duck_up(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32;
	self->takedamage = DAMAGE_AIM;
	gi.linkentity(self);
}

mframe_t zurkon_frames_duck[] =
{
	ai_move, 1, zurkon_duck_down,
	ai_move, 1, NULL,
	ai_move, 1, zurkon_duck_hold,
	ai_move, 0, NULL,
	ai_move, -1, NULL,
	ai_move, -1, NULL,
	ai_move, 0, zurkon_duck_up,
	ai_move, -1, NULL
};
mmove_t	zurkon_move_duck = { FRAME_duck01, FRAME_duck08, zurkon_frames_duck, zurkon_run };

void zurkon_dodge(edict_t *self, edict_t *attacker, float eta)
{
	if (random() > 0.25)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	self->monsterinfo.currentmove = &zurkon_move_duck;
}


void zurkon_opengun(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_open, 1, ATTN_IDLE, 0);
}

void ZurkonFire(edict_t *self)
{
	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	target;
	vec3_t	aim;
	int		flash_number;
	flash_number = MZ2_GUNNER_MACHINEGUN_1 + (self->s.frame - FRAME_attak216);

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);
	// project enemy back a bit and target there
	VectorCopy(self->enemy->s.origin, target);
	VectorMA(target, -0.2, self->enemy->velocity, target);
	target[2] += self->enemy->viewheight;

	VectorSubtract(target, start, aim);
	VectorNormalize(aim);
	monster_fire_bullet(self, start, aim, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}
/*
void GunnerGrenade(edict_t *self)
{
	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	aim;
	int		flash_number;

	if (self->s.frame == FRAME_attak105)
		flash_number = MZ2_GUNNER_GRENADE_1;
	else if (self->s.frame == FRAME_attak108)
		flash_number = MZ2_GUNNER_GRENADE_2;
	else if (self->s.frame == FRAME_attak111)
		flash_number = MZ2_GUNNER_GRENADE_3;
	else // (self->s.frame == FRAME_attak114)
		flash_number = MZ2_GUNNER_GRENADE_4;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);

	//FIXME : do a spread -225 -75 75 225 degrees around forward
	VectorCopy(forward, aim);

	monster_fire_grenade(self, start, aim, 50, 600, flash_number);
}
*/
mframe_t zurkon_frames_attack_chain[] =
{
	/*
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	*/
	ai_charge, 0, zurkon_opengun,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t zurkon_move_attack_chain = { FRAME_attak209, FRAME_attak215, zurkon_frames_attack_chain, zurkon_fire_chain };

mframe_t zurkon_frames_fire_chain[] =
{
	ai_charge, 0, ZurkonFire,
	ai_charge, 0, ZurkonFire,
	ai_charge, 0, ZurkonFire,
	ai_charge, 0, ZurkonFire,
	ai_charge, 0, ZurkonFire,
	ai_charge, 0, ZurkonFire,
	ai_charge, 0, ZurkonFire,
	ai_charge, 0, ZurkonFire
};
mmove_t zurkon_move_fire_chain = { FRAME_attak216, FRAME_attak223, zurkon_frames_fire_chain, zurkon_refire_chain };

mframe_t zurkon_frames_endfire_chain[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t zurkon_move_endfire_chain = { FRAME_attak224, FRAME_attak230, zurkon_frames_endfire_chain, zurkon_run };
/*
mframe_t zurkon_frames_attack_grenade[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	//ai_charge, 0, GunnerGrenade,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	//ai_charge, 0, GunnerGrenade,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	//ai_charge, 0, GunnerGrenade,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	//ai_charge, 0, GunnerGrenade,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t gunner_move_attack_grenade = { FRAME_attak101, FRAME_attak121, zurkon_frames_attack_grenade, zurkon_run };
*/
void zurkon_attack(edict_t *self)
{
	if (range(self, self->enemy) == RANGE_MELEE)
	{
		self->monsterinfo.currentmove = &zurkon_move_attack_chain;
	}
	else
	{
		//if (random() <= 0.5)
			//self->monsterinfo.currentmove = &gunner_move_attack_grenade;
		//else
			self->monsterinfo.currentmove = &zurkon_move_attack_chain;
	}
}

void zurkon_fire_chain(edict_t *self)
{
	self->monsterinfo.currentmove = &zurkon_move_fire_chain;
}

void zurkon_refire_chain(edict_t *self)
{
	if (self->enemy->health > 0)
	if (visible(self, self->enemy))
	if (random() <= 0.5)
	{
		self->monsterinfo.currentmove = &zurkon_move_fire_chain;
		return;
	}
	self->monsterinfo.currentmove = &zurkon_move_endfire_chain;
}

int zurkon_find_client(edict_t *self)
{
	edict_t* sp_client = G_Find(sp_client, FOFS(classname), "info_player_start");
	edict_t* mp_client = G_Find(mp_client, FOFS(classname), "info_player_deathmatch");
	float rad = 100.0f;
	vec3_t origin;
	vec3_t	eorg;
	edict_t* ent;
	origin[0] = self->s.origin[0];
	origin[1] = self->s.origin[1];
	origin[2] = self->s.origin[2];
	if (findradius(sp_client, origin, rad))
		ent = findradius(sp_client, origin, rad);
		return 1;
	if (findradius(mp_client, origin, rad))
		return 1;
	return 0;

}

void zurkon_think(edict_t *self)
{
	edict_t *ent;
	while (ent = findradius(ent, self->s.origin, 300) != NULL){
		if ((ent->svflags & SVF_MONSTER) && !ent->client && ent != self && ent->health > 1){
			self->activator = ent;
			self->enemy = ent;
			self->target = ent->classname;
		}
	}
	M_MoveFrame(self);
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround(self);
	}
	M_CatagorizePosition(self);
	M_WorldEffects(self);
	M_SetEffects(self);
}


/*QUAKED monster_gunner (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/

void SP_monster_zurkon(edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}
	edict_t* sp_client;
	sound_death = gi.soundindex("gunner/death1.wav");
	sound_pain = gi.soundindex("gunner/gunpain2.wav");
	sound_pain2 = gi.soundindex("gunner/gunpain1.wav");
	sound_idle = gi.soundindex("gunner/gunidle1.wav");
	sound_open = gi.soundindex("gunner/gunatck1.wav");
	sound_search = gi.soundindex("gunner/gunsrch1.wav");
	sound_sight = gi.soundindex("gunner/sight1.wav");

	gi.soundindex("gunner/gunatck2.wav");
	gi.soundindex("gunner/gunatck3.wav");

	gi.sound(self, CHAN_BODY, gi.soundindex("misc/zurkonspawn.wav"), 1, ATTN_NORM, 0);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/gunner/tris.md2");
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 32);
	self->health = 175;
	self->gib_health = -70;
	self->mass = 200;
	self->classname = "zurkon";
	self->pain = zurkon_pain;
	self->die = zurkon_die;
	self->ally = 1;
	//self->enemy = findradius(self, self->s.origin, 300.0f);
	//self->monsterinfo.aiflags |= AI_GOOD_GUY;
	//self->enemy = zurkon_find_enemy;
	//self->enemy = NULL;
	//self->movetarget = G_Find(sp_client,FOFS(classname), "info_player_start");
	self->monsterinfo.stand = zurkon_stand;
	self->monsterinfo.walk = zurkon_walk;
	self->monsterinfo.run = zurkon_run;
	self->monsterinfo.dodge = zurkon_dodge;
	self->monsterinfo.attack = zurkon_attack;
	self->monsterinfo.melee = NULL; 
	//self->team = "Team Ratchet"; 
	//self->goalentity = G_PickTarget(self->team);
	//self->monsterinfo.aiflags |= ~AI_COMBAT_POINT;
	self->monsterinfo.sight = zurkon_sight;
	self->monsterinfo.search = zurkon_search;
	//self->think = zurkon_think;
	gi.linkentity(self);

	self->monsterinfo.currentmove = &zurkon_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}
