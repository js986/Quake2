/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "g_local.h"


/*
=================
check_dodge

This is a support routine used when a client is firing
a non-instant attack weapon.  It checks to see if a
monster's dodge function should be called.
=================
*/
static void check_dodge (edict_t *self, vec3_t start, vec3_t dir, int speed)
{
	vec3_t	end;
	vec3_t	v;
	trace_t	tr;
	float	eta;

	// easy mode only ducks one quarter the time
	if (skill->value == 0)
	{
		if (random() > 0.25)
			return;
	}
	VectorMA (start, 8192, dir, end);
	tr = gi.trace (start, NULL, NULL, end, self, MASK_SHOT);
	if ((tr.ent) && (tr.ent->svflags & SVF_MONSTER) && (tr.ent->health > 0) && (tr.ent->monsterinfo.dodge) && infront(tr.ent, self))
	{
		VectorSubtract (tr.endpos, start, v);
		eta = (VectorLength(v) - tr.ent->maxs[0]) / speed;
		tr.ent->monsterinfo.dodge (tr.ent, self, eta);
	}
}


/*
=================
fire_hit

Used for all impact (hit/punch/slash) attacks
=================
*/
qboolean fire_hit (edict_t *self, vec3_t aim, int damage, int kick)
{
	trace_t		tr;
	vec3_t		forward, right, up;
	vec3_t		v;
	vec3_t		point;
	float		range;
	vec3_t		dir;

	//see if enemy is in range
	VectorSubtract (self->enemy->s.origin, self->s.origin, dir);
	range = VectorLength(dir);
	if (range > aim[0])
		return false;

	if (aim[1] > self->mins[0] && aim[1] < self->maxs[0])
	{
		// the hit is straight on so back the range up to the edge of their bbox
		range -= self->enemy->maxs[0];
	}
	else
	{
		// this is a side hit so adjust the "right" value out to the edge of their bbox
		if (aim[1] < 0)
			aim[1] = self->enemy->mins[0];
		else
			aim[1] = self->enemy->maxs[0];
	}

	VectorMA (self->s.origin, range, dir, point);

	tr = gi.trace (self->s.origin, NULL, NULL, point, self, MASK_SHOT);
	if (tr.fraction < 1)
	{
		if (!tr.ent->takedamage)
			return false;
		// if it will hit any client/monster then hit the one we wanted to hit
		if ((tr.ent->svflags & SVF_MONSTER) || (tr.ent->client))
			tr.ent = self->enemy;
	}

	AngleVectors(self->s.angles, forward, right, up);
	VectorMA (self->s.origin, range, forward, point);
	VectorMA (point, aim[1], right, point);
	VectorMA (point, aim[2], up, point);
	VectorSubtract (point, self->enemy->s.origin, dir);

	// do the damage
	T_Damage (tr.ent, self, self, dir, point, vec3_origin, damage, kick/2, DAMAGE_NO_KNOCKBACK, MOD_HIT);

	if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client))
		return false;

	// do our special form of knockback here
	VectorMA (self->enemy->absmin, 0.5, self->enemy->size, v);
	VectorSubtract (v, point, v);
	VectorNormalize (v);
	VectorMA (self->enemy->velocity, kick, v, self->enemy->velocity);
	if (self->enemy->velocity[2] > 0)
		self->enemy->groundentity = NULL;
	return true;
}


/*
=================
fire_lead

This is an internal support routine used for bullet/pellet based weapons.
=================
*/
static void fire_lead (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, int mod)
{
	trace_t		tr;
	vec3_t		dir;
	vec3_t		forward, right, up;
	vec3_t		end;
	float		r;
	float		u;
	vec3_t		offset_start;
	vec3_t		fire_pos;
	vec3_t		water_start;
	qboolean	water = false;
	int			content_mask = MASK_SHOT | MASK_WATER;

	tr = gi.trace (self->s.origin, NULL, NULL, start, self, MASK_SHOT);
	if (!(tr.fraction < 1.0))
	{
		vectoangles (aimdir, dir);
		AngleVectors (dir, forward, right, up);
		/*
		if (self->client && (self->chaingun == 1)){
			edict_t	*fireball;

			fireball = G_Spawn();
			VectorCopy(start, fireball->s.origin);
			VectorCopy(dir, fireball->movedir);
			vectoangles(dir, fireball->s.angles);
			VectorScale(aimdir, 30, fireball->velocity);
			fireball->movetype = MOVETYPE_FLY;
			fireball->clipmask = MASK_SHOT;
			fireball->solid = SOLID_BBOX;
			fireball->s.effects |= EF_ANIM23;
			VectorClear(fireball->mins);
			VectorClear(fireball->maxs);
			fireball->s.modelindex = gi.modelindex("sprites/s_explo2.sp2");
			fireball->owner = self;
			//fireball->touch = bfg_touch;
			fireball->nextthink = level.time + 3;
			fireball->think = G_FreeEdict;
			fireball->radius_dmg = damage;
			//bfg->dmg_radius = damage_radius;
			fireball->classname = "fireball blast";
			gi.linkentity(fireball);
			self->chaingun = 0;
			return;
		}
	*/
		r = crandom()*hspread;
		u = crandom()*vspread;
		VectorMA (start, 8192, forward, end);
		VectorMA (end, r, right, end);
		VectorMA (end, u, up, end);

		if (gi.pointcontents (start) & MASK_WATER)
		{
			water = true;
			VectorCopy (start, water_start);
			content_mask &= ~MASK_WATER;
		}

		tr = gi.trace (start, NULL, NULL, end, self, content_mask);
		//fire_rocket(self, start, dir, 100, 3000, 50, 50);
		VectorSet(offset_start, 0, 30, 0);
		VectorAdd(offset_start,start, offset_start);
		VectorAdd(start, tr.endpos, fire_pos);
		VectorScale(fire_pos, 0.5, fire_pos);
		if (self->client && (self->chaingun == 1)){
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_EXPLOSION1);
			//gi.WriteShort(self - g_edicts);
			gi.WritePosition(tr.endpos);
			//gi.WritePosition(tr.endpos);
			//gi.WriteAngle(90);
			//gi.WriteDir(tr.plane.normal);
			gi.multicast(tr.endpos, MULTICAST_PVS);
		}

		if (self->client && (self->supershot == 1)){
			if ((tr.ent != self) && (tr.ent->takedamage) && (tr.ent->svflags & SVF_MONSTER) && tr.ent->health){
				//int odds = (rand() % (100 - 2 + 1)) + 2;
				//if (odds > 50);
				tr.ent->monsterinfo.shot_super = 1;
				tr.ent->teammaster = self;
				tr.ent->ally = 1;
				return;
			}
		}

		
		// see if we hit water
		if (tr.contents & MASK_WATER)
		{
			int		color;

			water = true;
			VectorCopy (tr.endpos, water_start);

			if (!VectorCompare (start, tr.endpos))
			{
				if (tr.contents & CONTENTS_WATER)
				{
					if (strcmp(tr.surface->name, "*brwater") == 0)
						color = SPLASH_BROWN_WATER;
					else
						color = SPLASH_BLUE_WATER;
				}
				else if (tr.contents & CONTENTS_SLIME)
					color = SPLASH_SLIME;
				else if (tr.contents & CONTENTS_LAVA)
					color = SPLASH_LAVA;
				else
					color = SPLASH_UNKNOWN;

				if (color != SPLASH_UNKNOWN)
				{
					gi.WriteByte(svc_temp_entity);
					gi.WriteByte(TE_SPLASH);
					gi.WriteByte (8);
					gi.WritePosition (tr.endpos);
					gi.WriteDir (tr.plane.normal);
					gi.WriteByte (color);
					gi.multicast (tr.endpos, MULTICAST_PVS);
				}

				// change bullet's course when it enters water
				VectorSubtract (end, start, dir);
				vectoangles (dir, dir);
				AngleVectors (dir, forward, right, up);
				r = crandom()*hspread*2;
				u = crandom()*vspread*2;
				VectorMA (water_start, 8192, forward, end);
				VectorMA (end, r, right, end);
				VectorMA (end, u, up, end);
			}

			// re-trace ignoring water this time
			tr = gi.trace (water_start, NULL, NULL, end, self, MASK_SHOT);
		}
	}

	// send gun puff / flash
	if (!((tr.surface) && (tr.surface->flags & SURF_SKY)))
	{
		if (tr.fraction < 1.0)
		{
			if (tr.ent->takedamage)
			{
				if (self->client && self->chaingun == 1){
					T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, 50, kick, DAMAGE_BULLET, mod);
					self->chaingun = 0;
				}
				else
					T_Damage (tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, DAMAGE_BULLET, mod);
			}
			else
			{
				if (strncmp (tr.surface->name, "sky", 3) != 0)
				{
					gi.WriteByte (svc_temp_entity);
					gi.WriteByte (te_impact);
					gi.WritePosition (tr.endpos);
					gi.WriteDir (tr.plane.normal);
					gi.multicast (tr.endpos, MULTICAST_PVS);

					if (self->client)
						PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
				}
			}
		}
	}

	// if went through water, determine where the end and make a bubble trail
	if (water)
	{
		vec3_t	pos;

		VectorSubtract (tr.endpos, water_start, dir);
		VectorNormalize (dir);
		VectorMA (tr.endpos, -2, dir, pos);
		if (gi.pointcontents (pos) & MASK_WATER)
			VectorCopy (pos, tr.endpos);
		else
			tr = gi.trace (pos, NULL, NULL, water_start, tr.ent, MASK_WATER);

		VectorAdd (water_start, tr.endpos, pos);
		VectorScale (pos, 0.5, pos);

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BUBBLETRAIL);
		gi.WritePosition (water_start);
		gi.WritePosition (tr.endpos);
		gi.multicast (pos, MULTICAST_PVS);
	}
}


/*
=================
fire_bullet

Fires a single round.  Used for machinegun and chaingun.  Would be fine for
pistols, rifles, etc....
=================
*/
void fire_bullet (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int mod)
{
	fire_lead (self, start, aimdir, damage, kick, TE_GUNSHOT, hspread, vspread, mod);
}


/*
=================
fire_shotgun

Shoots shotgun pellets.  Used by shotgun and super shotgun.
=================
*/
void fire_shotgun (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, int mod)
{
	int		i;
	for (i = 0; i < count; i++)
		fire_lead (self, start, aimdir, damage, kick, TE_SHOTGUN|TE_SHIELD_SPARKS, hspread, vspread, mod);
}


/*
=================
fire_blaster

Fires a single blaster bolt.  Used by the blaster and hyper blaster.
=================
*/
void blaster_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int		mod;

	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
	{
		if (self->spawnflags & 1)
			mod = MOD_HYPERBLASTER;
		else
			mod = MOD_BLASTER;
		T_Damage (other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, 1, DAMAGE_ENERGY, mod);
	}
	else
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BLASTER);
		gi.WritePosition (self->s.origin);
		if (!plane)
			gi.WriteDir (vec3_origin);
		else
			gi.WriteDir (plane->normal);
		gi.multicast (self->s.origin, MULTICAST_PVS);
	}

	G_FreeEdict (self);
}

void fire_blaster (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect, qboolean hyper)
{
	edict_t	*bolt;
	trace_t	tr;
	vec3_t vec;
	edict_t *temp;
	edict_t *zurkon;
	VectorNormalize (dir);
	/*
	if (self->client && hyper == false && self->zurkon == 0){
		zurkon = G_Spawn();
		zurkon->movetarget = self;
		//zurkon->s.origin[2] += 10.0f;
		//VectorCopy(start, zurkon->s.origin);
		//VectorCopy(start, zurkon->s.old_origin);
		self->zurkon = 1;
		zurkon->teammaster = self;
		//SP_monster_zurkon(zurkon);
	}
	*/
	bolt = G_Spawn();
	bolt->svflags = SVF_DEADMONSTER;
	// yes, I know it looks weird that projectiles are deadmonsters
	// what this means is that when prediction is used against the object
	// (blaster/hyperblaster shots), the player won't be solid clipped against
	// the object.  Right now trying to run into a firing hyperblaster
	// is very jerky since you are predicted 'against' the shots.
	VectorCopy (start, bolt->s.origin);
	VectorCopy (start, bolt->s.old_origin);
	vectoangles (dir, bolt->s.angles);
	VectorScale (dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE|MOVETYPE_BOUNCE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= effect;
	VectorClear (bolt->mins);
	VectorClear (bolt->maxs);
	bolt->s.modelindex = gi.modelindex ("models/objects/laser/tris.md2");
	bolt->s.sound = gi.soundindex ("misc/lasfly.wav");
	bolt->owner = self;
	bolt->touch = blaster_touch;
	bolt->nextthink = level.time + 2;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	bolt->classname = "bolt";
	if (hyper)
		bolt->spawnflags = 1;
	gi.linkentity (bolt);

	if (self->client)
		check_dodge (self, bolt->s.origin, dir, speed);
	tr = gi.trace (self->s.origin, NULL, NULL, bolt->s.origin, bolt, MASK_SHOT);
	//ztr = gi.trace(zurkon->s.origin, NULL, NULL, zurkon->s.origin, zurkon, CONTENTS_MONSTER);
	if (self->client && hyper == false && self->zurkon == 0){
		//VectorMA(zurkon->s.origin, -10, dir, zurkon->s.origin);
		zurkon = G_Spawn();
		zurkon->movetarget = self;
		self->zurkon = 1;
		zurkon->teammaster = self;
		//zurkon->target = "monster";
		VectorSet(vec, 0, 30, 0);
		VectorAdd(vec, tr.endpos, vec);
		VectorCopy(vec, zurkon->s.origin);
		VectorCopy(vec, zurkon->s.old_origin);
		//gi.linkentity(zurkon);
		SP_monster_zurkon(zurkon);
	}
	if (tr.fraction < 1.0)
	{
		VectorMA (bolt->s.origin, -10, dir, bolt->s.origin);
		bolt->touch (bolt, tr.ent, NULL, NULL);
	}
}	


/*
=================
fire_grenade
=================
*/
static void Bee_Mine_Glove(edict_t *self)
{
	int			mod;
	edict_t		*grenade2;
	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	//FIXME: if we are onground then raise our Z just a bit since we are a point?
	if (self->enemy)
	{
		float	points;
		vec3_t	v;
		vec3_t	dir;

		VectorAdd(self->enemy->mins, self->enemy->maxs, v);
		VectorMA(self->enemy->s.origin, 0.5, v, v);
		VectorSubtract(self->s.origin, v, v);
		points = self->dmg - 0.5 * VectorLength(v);
		VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
		if (self->spawnflags & 1)
			mod = MOD_HANDGRENADE;
		else
			mod = MOD_GRENADE;
		T_Damage(self->enemy, self, self->owner, dir, self->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
	}
	self->bee_hive++;
	T_RadiusDamage(self, self->owner, self->dmg, self->enemy, self->dmg_radius, mod);
	//gi.cprintf(self->owner, PRINT_MEDIUM, "Bee Hive: %d \n",self->bee_hive);
	if (self->bee_hive == self->bee_level+7)
		G_FreeEdict(self);
	self->nextthink = level.time + 1;
	//VectorMA(ent->s.origin, -0.02, ent->velocity, origin);
	
}



static void Grenade_Explode (edict_t *ent)
{
	vec3_t		origin;
	int			mod;
	edict_t		*grenade2;
	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	//FIXME: if we are onground then raise our Z just a bit since we are a point?
	if (ent->enemy)
	{
		float	points;
		vec3_t	v;
		vec3_t	dir;

		VectorAdd (ent->enemy->mins, ent->enemy->maxs, v);
		VectorMA (ent->enemy->s.origin, 0.5, v, v);
		VectorSubtract (ent->s.origin, v, v);
		points = ent->dmg - 0.5 * VectorLength (v);
		VectorSubtract (ent->enemy->s.origin, ent->s.origin, dir);
		if (ent->spawnflags & 1)
			mod = MOD_HANDGRENADE;
		else
			mod = MOD_GRENADE;
		T_Damage (ent->enemy, ent, ent->owner, dir, ent->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
	}

	if (ent->spawnflags & 2)
		mod = MOD_HELD_GRENADE;
	else if (ent->spawnflags & 1)
		mod = MOD_HG_SPLASH;
	else
		mod = MOD_G_SPLASH;
	T_RadiusDamage(ent, ent->owner, ent->dmg, ent->enemy, ent->dmg_radius, mod);

	VectorMA (ent->s.origin, -0.02, ent->velocity, origin);
	
	if (ent->owner->client && !(ent->spawnflags & 1)){
		int num_grenades = 5 + ent->owner->bouncer_level;
		for (int i = 0; i < num_grenades; i++) {
		grenade2 = G_Spawn();
		vec3_t aimdir;
		vec3_t	dir;
		vec3_t	forward, right, up;
		vec3_t  dir1;
		trace_t trace;
		int mask;
			VectorSet(aimdir,crandom() *10.0, 10, crandom() * 10.0);
			vectoangles(aimdir, dir);
			AngleVectors(dir, forward, right, up);
			dir1[0] = 1.0f;
			dir1[1] = 1.0f;
			dir1[2] = 1.0f;
			VectorCopy(origin, grenade2->s.origin);
			//VectorScale(aimdir, speed, grenade2->velocity);
			VectorMA(grenade2->velocity, 200 + crandom() * 10.0, up, grenade2->velocity);
			VectorMA(grenade2->velocity, crandom() * 10.0, right, grenade2->velocity);
			VectorSet(grenade2->avelocity, 300, 300, 300);
			grenade2->movetype = MOVETYPE_BOUNCE;
			grenade2->clipmask = MASK_SHOT;
			grenade2->solid = SOLID_BBOX;
			grenade2->s.effects |= EF_GRENADE;
			VectorClear(grenade2->mins);
			VectorClear(grenade2->maxs);
			grenade2->s.modelindex = gi.modelindex("models/objects/grenade2/tris.md2");
			grenade2->owner = ent;
			//grenade2->touch = Grenade_Touch;
			grenade2->nextthink = level.time + 1.5;
			grenade2->think = Grenade_Explode;
			grenade2->dmg = 70;
			grenade2->dmg_radius = 100;
			grenade2->classname = "hgrenade";
			gi.linkentity(grenade2);
		}
	}
	
	gi.WriteByte (svc_temp_entity);
	if (ent->waterlevel)
	{
		if (ent->groundentity)
			gi.WriteByte (TE_GRENADE_EXPLOSION_WATER);
		else
			gi.WriteByte (TE_ROCKET_EXPLOSION_WATER);
	}
	else
	{
		if (ent->groundentity)
			gi.WriteByte (TE_GRENADE_EXPLOSION);
		else
			gi.WriteByte (TE_ROCKET_EXPLOSION);
	}

	gi.WritePosition (origin);
	gi.multicast (ent->s.origin, MULTICAST_PHS);

	G_FreeEdict (ent);
}

static void Grenade_Touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (ent);
		return;
	}

	if (!other->takedamage)
	{
		if (ent->spawnflags & 1)
		{
			if (random() > 0.5)
				gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/hgrenb1a.wav"), 1, ATTN_NORM, 0);
			else
				gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/hgrenb2a.wav"), 1, ATTN_NORM, 0);
		}
		else
		{
			gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/grenlb1b.wav"), 1, ATTN_NORM, 0);
		}
		return;
	}

	ent->enemy = other;
	Grenade_Explode (ent);
}

void fire_grenade (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius)
{
	edict_t	*grenade;
	edict_t *grenade2;
	vec3_t	dir;
	vec3_t	forward, right, up;
	trace_t trace;
	int mask;


	vectoangles (aimdir, dir);
	AngleVectors (dir, forward, right, up);

	grenade = G_Spawn();
	VectorCopy (start, grenade->s.origin);
	VectorScale (aimdir, speed, grenade->velocity);
	VectorMA (grenade->velocity, 200 + crandom() * 10.0, up, grenade->velocity);
	VectorMA (grenade->velocity, crandom() * 10.0, right, grenade->velocity);
	VectorSet (grenade->avelocity, 300, 300, 300);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->clipmask = MASK_SHOT;
	grenade->solid = SOLID_BBOX;
	grenade->s.effects |= EF_GRENADE;
	VectorClear (grenade->mins);
	VectorClear (grenade->maxs);
	grenade->s.modelindex = gi.modelindex ("models/objects/grenade/tris.md2");
	grenade->owner = self;
	grenade->touch = Grenade_Touch;
	grenade->nextthink = level.time + timer;
	if (self->clipmask)
		mask = self->clipmask;
	trace = gi.trace(start, self->mins, self->maxs, self->s.origin, grenade, mask);
	grenade->think = Grenade_Explode;
	grenade->dmg = damage;
	grenade->dmg_radius = damage_radius;
	grenade->classname = "grenade";

	

	gi.linkentity (grenade);
}

void fire_grenade2 (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius, qboolean held)
{
	edict_t	*grenade;
	vec3_t	dir;
	vec3_t	forward, right, up;
	vec3_t	size;
	
	vectoangles (aimdir, dir);
	AngleVectors (dir, forward, right, up);

	grenade = G_Spawn();
	VectorCopy (start, grenade->s.origin);
	VectorScale (aimdir, speed, grenade->velocity);
	//VectorSet(size, 1000, 1000, 1000);
	//VectorCopy(size, grenade->size);
	VectorMA (grenade->velocity, 200 + crandom() * 10.0, up, grenade->velocity);
	VectorMA (grenade->velocity, crandom() * 10.0, right, grenade->velocity);
	VectorSet (grenade->avelocity, 300, 300, 300);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->clipmask = MASK_SHOT;
	grenade->solid = SOLID_BBOX;
	grenade->s.effects |= EF_GRENADE;
	if (self->client){
		//grenade->s.effects |= EF_FLIES;
		grenade->s.effects |= EF_COLOR_SHELL;
		grenade->s.renderfx |= RF_SHELL_GREEN | RF_SHELL_RED;
	}
	VectorClear (grenade->mins);
	VectorClear (grenade->maxs);
	grenade->s.modelindex = gi.modelindex ("models/objects/grenade2/tris.md2");
	if (self->client) {
		grenade->owner = self;
		grenade->bee_hive = 1;
		grenade->nextthink = level.time + 1;
		grenade->think = Bee_Mine_Glove;
		grenade->dmg = 50;
		grenade->dmg_radius = 100;
		grenade->classname = "hgrenade";
		grenade->s.effects |= EF_FLIES;
		grenade->s.effects |= EF_BFG;
		//grenade->s.sound = gi.soundindex("infantry/inflies1.wav");
	}
	else{
		grenade->owner = self;
		grenade->touch = Grenade_Touch;
		grenade->nextthink = level.time + timer;
		grenade->think = Grenade_Explode;
		grenade->dmg = damage;
		grenade->dmg_radius = damage_radius;
		grenade->classname = "hgrenade";
	}
	if (held)
		grenade->spawnflags = 3;
	else
		grenade->spawnflags = 1;
	if (self->client)
		grenade->s.sound = gi.soundindex("infantry/inflies1.wav");
	else
		grenade->s.sound = gi.soundindex("weapons/hgrenc1b.wav");

	if (timer <= 0.0)
		Grenade_Explode (grenade);
	else
	{
		if (self->client){
			gi.sound(self, CHAN_WEAPON, gi.soundindex("infantry/inflies1.wav"), 1, ATTN_NORM, 0);
		}
		else
			gi.sound (self, CHAN_WEAPON, gi.soundindex ("weapons/hgrent1a.wav"), 1, ATTN_NORM, 0);
		gi.linkentity (grenade);
	}
}


/*
=================
fire_rocket
=================
*/
void rocket_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t		origin;
	int			n;

	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (ent);
		return;
	}

	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	// calculate position for the explosion entity
	VectorMA (ent->s.origin, -0.02, ent->velocity, origin);

	if (other->takedamage)
	{
		T_Damage (other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 0, 0, MOD_ROCKET);
	}
	else
	{
		// don't throw any debris in net games
		if (!deathmatch->value && !coop->value)
		{
			if ((surf) && !(surf->flags & (SURF_WARP|SURF_TRANS33|SURF_TRANS66|SURF_FLOWING)))
			{
				n = rand() % 5;
				while(n--)
					ThrowDebris (ent, "models/objects/debris2/tris.md2", 2, ent->s.origin);
			}
		}
	}

	T_RadiusDamage(ent, ent->owner, ent->radius_dmg, other, ent->dmg_radius, MOD_R_SPLASH);

	gi.WriteByte (svc_temp_entity);
	if (ent->waterlevel)
		gi.WriteByte (TE_ROCKET_EXPLOSION_WATER);
	else
		gi.WriteByte (TE_ROCKET_EXPLOSION);
	gi.WritePosition (origin);
	gi.multicast (ent->s.origin, MULTICAST_PHS);

	G_FreeEdict (ent);
}

void fire_rocket (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius, int radius_damage)
{
	edict_t	*rocket;

	rocket = G_Spawn();
	VectorCopy (start, rocket->s.origin);
	VectorCopy (dir, rocket->movedir);
	vectoangles (dir, rocket->s.angles);
	VectorScale (dir, speed, rocket->velocity);
	rocket->movetype = MOVETYPE_FLYMISSILE;
	rocket->clipmask = MASK_SHOT;
	rocket->solid = SOLID_BBOX;
	rocket->s.effects |= EF_ROCKET;
	VectorClear (rocket->mins);
	VectorClear (rocket->maxs);
	rocket->s.modelindex = gi.modelindex ("models/objects/rocket/tris.md2");
	rocket->owner = self;
	rocket->touch = rocket_touch;
	rocket->nextthink = level.time + 8000/speed;
	rocket->think = G_FreeEdict;
	rocket->dmg = damage;
	rocket->radius_dmg = radius_damage;
	rocket->dmg_radius = damage_radius;
	rocket->s.sound = gi.soundindex ("weapons/rockfly.wav");
	rocket->classname = "rocket";

	if (self->client)
		check_dodge (self, rocket->s.origin, dir, speed);

	gi.linkentity (rocket);
}


/*
=================
fire_rail
=================
*/
edict_t *FindMonster(edict_t *self)
{
	edict_t	*ent = NULL;
	edict_t	*best = NULL;

	while (ent = findradius(ent, self->s.origin, 500) && ent != NULL)
	{
		if (ent != self && (ent->svflags & SVF_MONSTER) && !ent->client){
			best = ent;
			return best;
		}

		/*
		if (ent == self)
		continue;
		if (!(ent->takedamage))
		continue;
		if (!ent->health)
		continue;
		if (ent->health < 1)
		continue;
		//if (!visible(self, ent))
		//	continue;
		if (!best)
		{
		best = ent;
		continue;
		}
		//if (ent->max_health == best->max_health)
		//continue;
		//best = ent;
		}
		//gi.dprintf("Found Something!!!");
		//gi.dprintf(best->classname);
		return best;
		*/
	}
}
void fire_rail (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick)
{
	vec3_t		from;
	vec3_t		end;
	trace_t		tr;
	edict_t		*ignore;
	int			mask;
	qboolean	water;

	edict_t		*grappling_hook;

	VectorMA (start, 8192, aimdir, end);
	VectorCopy (start, from);
	ignore = self;
	water = false;
	mask = MASK_SHOT|CONTENTS_SLIME|CONTENTS_LAVA;
	while (ignore)
	{
		tr = gi.trace (from, NULL, NULL, end, ignore, mask);

		if (tr.contents & (CONTENTS_SLIME|CONTENTS_LAVA))
		{
			mask &= ~(CONTENTS_SLIME|CONTENTS_LAVA);
			water = true;
		}
		else
		{
			if ((tr.ent->svflags & SVF_MONSTER) || (tr.ent->client))
				ignore = tr.ent;
			else
				ignore = NULL;

			if ((tr.ent != self) && (tr.ent->takedamage)){
				mmove_t* savemove = tr.ent->monsterinfo.currentmove;
				int save_nextthink = tr.ent->nextthink;
				float savedelay = tr.ent->delay;
				if (self->client && (tr.ent->svflags & SVF_MONSTER) && tr.ent->health){
					//tr.ent->activator = monster;
					tr.ent->monsterinfo.shot_rail = 1;
					tr.ent->who_shot_me = self;
					//tr.ent->nextthink = level.time + 3;
					//tr.ent->delay = 3;
					//tr.ent->monsterinfo.currentmove = savemove;
					//tr.ent->delay = savedelay;
					//tr.ent->nextthink = save_nextthink;
					//tr.ent->nextthink = level.time + 3;
					//tr.ent->monsterinfo.aiflags |= AI_GOOD_GUY;
					//tr.ent->monsterinfo.aiflags |= AI_HOLD_FRAME;
					//tr.ent->enemy = monster;
					//tr.ent->target = monster->classname;
					//gi.dprintf(monster->classname);
					//tr.ent->goalentity = monster;
					//tr.ent->nextthink = level.time + 5;
					//tr.ent->think;
					//tr.ent->nextthink = save_nextthink;
				}
				else
					T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, 0, MOD_RAILGUN);
			}
		}

		VectorCopy (tr.endpos, from);
	}
	vec3_t v;
	if (self->client){
		/*
		grappling_hook = G_Spawn();
		VectorCopy(start, grappling_hook->s.origin);
			//VectorAdd(v, tr.endpos, self->s.origin);
		//gi.linkentity(self);
		grappling_hook->s.modelindex = gi.modelindex("models/weapons/grapple/hook/tris.md2");
		grappling_hook->owner = self;
		VectorScale(aimdir, 50, grappling_hook->velocity);
		grappling_hook->movetype = MOVETYPE_FLYMISSILE;
		grappling_hook->clipmask = MASK_SHOT;
		grappling_hook->solid = SOLID_BBOX;
		//grapple->s.effects |= effect;
		VectorClear(grappling_hook->mins);
		VectorClear(grappling_hook-> maxs);
		VectorCopy(grappling_hook->s.origin, grappling_hook->moveinfo.start_origin);
		VectorCopy(tr.endpos, grappling_hook->moveinfo.end_origin);
		//grappling_hook->think = G_FreeEdict;
		//grappling_hook->nextthink = level.time + 3;
		VectorMA(grappling_hook->s.origin, -10, aimdir, grappling_hook->s.origin);
		gi.linkentity(grappling_hook);
		return;
		*/
	}
	// send gun puff / flash
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_RAILTRAIL);
	gi.WritePosition (start);
	gi.WritePosition (tr.endpos);
	gi.multicast (self->s.origin, MULTICAST_PHS);
	if (water)
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_RAILTRAIL);
		gi.WritePosition (start);
		gi.WritePosition (tr.endpos);
		gi.multicast (tr.endpos, MULTICAST_PHS);
	}

	if (self->client)
		PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
}


/*
=================
fire_bfg
=================
*/
void bfg_explode (edict_t *self)
{
	edict_t	*ent;
	float	points;
	vec3_t	v;
	float	dist;

	if (self->s.frame == 0)
	{
		// the BFG effect
		ent = NULL;
		while ((ent = findradius(ent, self->s.origin, self->dmg_radius)) != NULL)
		{
			if (!ent->takedamage)
				continue;
			if (ent == self->owner)
				continue;
			if (!CanDamage (ent, self))
				continue;
			if (!CanDamage (ent, self->owner))
				continue;

			VectorAdd (ent->mins, ent->maxs, v);
			VectorMA (ent->s.origin, 0.5, v, v);
			VectorSubtract (self->s.origin, v, v);
			dist = VectorLength(v);
			points = self->radius_dmg * (1.0 - sqrt(dist/self->dmg_radius));
			if (ent == self->owner)
				points = points * 0.5;

			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_BFG_EXPLOSION);
			gi.WritePosition (ent->s.origin);
			gi.multicast (ent->s.origin, MULTICAST_PHS);
			T_Damage (ent, self, self->owner, self->velocity, ent->s.origin, vec3_origin, (int)points, 0, DAMAGE_ENERGY, MOD_BFG_EFFECT);
		}
	}

	self->nextthink = level.time + FRAMETIME;
	self->s.frame++;
	if (self->s.frame == 5)
		self->think = G_FreeEdict;
}

void bfg_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	// core explosion - prevents firing it into the wall/floor
	if (other->takedamage)
		T_Damage (other, self, self->owner, self->velocity, self->s.origin, plane->normal, 200, 0, 0, MOD_BFG_BLAST);
	T_RadiusDamage(self, self->owner, 200, other, 100, MOD_BFG_BLAST);

	//gi.sound (self, CHAN_VOICE, gi.soundindex ("weapons/bfg__x1b.wav"), 1, ATTN_NORM, 0);
	gi.sound(self, CHAN_VOICE, gi.soundindex("misc/overture.wav"), 1, ATTN_NORM, 0);
	self->solid = SOLID_NOT;
	self->touch = NULL;
	VectorMA (self->s.origin, -1 * FRAMETIME, self->velocity, self->s.origin);
	VectorClear (self->velocity);
	self->s.modelindex = gi.modelindex ("sprites/s_bfg3.sp2");
	self->s.frame = 0;
	self->s.sound = 0;
	self->s.effects &= ~EF_ANIM_ALLFAST;
	self->think = bfg_explode;
	self->nextthink = level.time + FRAMETIME;
	self->enemy = other;

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BFG_BIGEXPLOSION);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);
}


void bfg_think (edict_t *self)
{
	edict_t	*ent;
	edict_t	*ignore;
	vec3_t	point;
	vec3_t	dir;
	vec3_t	start;
	vec3_t	end;
	int		dmg;
	trace_t	tr;

	if (deathmatch->value)
		dmg = 5;
	else
		dmg = 10;

	ent = NULL;
	while ((ent = findradius(ent, self->s.origin, 256)) != NULL)
	{
		if (ent == self)
			continue;

		if (ent == self->owner)
			continue;

		if (!ent->takedamage)
			continue;

		if (!(ent->svflags & SVF_MONSTER) && (!ent->client) && (strcmp(ent->classname, "misc_explobox") != 0))
			continue;

		VectorMA (ent->absmin, 0.5, ent->size, point);

		VectorSubtract (point, self->s.origin, dir);
		VectorNormalize (dir);

		ignore = self;
		VectorCopy (self->s.origin, start);
		VectorMA (start, 2048, dir, end);
		while(1)
		{
			tr = gi.trace (start, NULL, NULL, end, ignore, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_DEADMONSTER);

			if (!tr.ent)
				break;

			// hurt it if we can
			if ((tr.ent->takedamage) && !(tr.ent->flags & FL_IMMUNE_LASER) && (tr.ent != self->owner))
				T_Damage (tr.ent, self, self->owner, dir, tr.endpos, vec3_origin, dmg, 1, DAMAGE_ENERGY, MOD_BFG_LASER);

			// if we hit something that's not a monster or player we're done
			if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client))
			{
				gi.WriteByte (svc_temp_entity);
				gi.WriteByte (TE_LASER_SPARKS);
				gi.WriteByte (4);
				gi.WritePosition (tr.endpos);
				gi.WriteDir (tr.plane.normal);
				gi.WriteByte (self->s.skinnum);
				gi.multicast (tr.endpos, MULTICAST_PVS);
				break;
			}

			ignore = tr.ent;
			VectorCopy (tr.endpos, start);
		}

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BFG_LASER);
		gi.WritePosition (self->s.origin);
		gi.WritePosition (tr.endpos);
		gi.multicast (self->s.origin, MULTICAST_PHS);
	}

	self->nextthink = level.time + FRAMETIME;
}


void fire_bfg (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius)
{
	int num_attacks = 6;
	edict_t	*bfg;
	vec3_t random;
	bfg = G_Spawn();
	VectorCopy (start, bfg->s.origin);
	VectorCopy (dir, bfg->movedir);
	vectoangles (dir, bfg->s.angles);
	VectorScale (dir, speed, bfg->velocity);
	bfg->movetype = MOVETYPE_FLYMISSILE;
	bfg->clipmask = MASK_SHOT;
	bfg->solid = SOLID_BBOX;
	bfg->s.effects |= EF_BFG | EF_ANIM_ALLFAST;
	VectorClear (bfg->mins);
	VectorClear (bfg->maxs);
	bfg->s.modelindex = gi.modelindex ("sprites/s_bfg1.sp2");
	bfg->owner = self;
	bfg->touch = bfg_touch;
	/*
	for (int i = 0; i < num_attacks; i++){
		VectorSet(random, 0, crandom() * 10, 0);
		VectorAdd(random, dir, random);
		fire_rocket(bfg, bfg->s.origin, random, speed, damage, damage_radius, damage);
		fire_lead(bfg, bfg->s.origin, random, damage, 3, TE_SHOTGUN, 70, 70, MOD_SHOTGUN);
	}
	*/
	//fire_lead(bfg, bfg->s.origin, random, damage, 3, TE_SHOTGUN, 70, 70, MOD_SHOTGUN);
	//fire_lead(bfg, bfg->s.origin, random, damage, 3, TE_SHOTGUN, 70, 70, MOD_SHOTGUN);
	//fire_lead(bfg, bfg->s.origin, random, damage, 3, TE_SHOTGUN, 70, 70, MOD_SHOTGUN);
	bfg->nextthink = level.time + 8000/speed;
	bfg->think = G_FreeEdict;
	bfg->radius_dmg = damage;
	bfg->dmg_radius = damage_radius;
	bfg->classname = "bfg blast";
	//bfg->s.sound = gi.soundindex ("weapons/bfg__l1a.wav");
	bfg->s.sound = gi.soundindex("misc/overture.wav");
	//gi.sound(self, CHAN_WEAPON, gi.soundindex("misc/overture.wav"), 1, ATTN_NORM, 0);
	
	bfg->think = bfg_think;
	bfg->nextthink = level.time + FRAMETIME;
	bfg->teammaster = bfg;
	bfg->teamchain = NULL;

	if (self->client)
		check_dodge (self, bfg->s.origin, dir, speed);

	gi.linkentity (bfg);
}
