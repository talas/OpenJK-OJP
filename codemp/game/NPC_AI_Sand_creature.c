/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2003 - 2008, OJP contributors

This file is part of the OpenJK-OJP source code.

OpenJK-OJP is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

//[NPCSandCreature]
#include "b_local.h"

extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
extern void G_SoundOnEnt(gentity_t *ent, int channel, const char *soundPath);
//extern float Q_flrand(float min, float max);
extern void G_GetBoltPosition(gentity_t *self, int boltIndex, vec3_t pos, int modelIndix);
extern void Rancor_DropVictim( gentity_t *self );//wahoo - :p

#define MIN_ATTACK_DIST_SQ	128
#define MIN_MISS_DIST		100
#define MIN_MISS_DIST_SQ	(MIN_MISS_DIST*MIN_MISS_DIST)
#define MAX_MISS_DIST		500
#define MAX_MISS_DIST_SQ	(MAX_MISS_DIST*MAX_MISS_DIST)
#define MIN_SCORE			-37500 //speed of (50*50) - dist of (200*200)

void SandCreature_Precache( void )
{
	int i;
	G_EffectIndex( "env/sand_dive" );
	G_EffectIndex( "env/sand_spray" );
	G_EffectIndex( "env/sand_move" );
	G_EffectIndex( "env/sand_move_breach" );
	//G_EffectIndex( "env/sand_attack_breach" );
	for ( i = 1; i < 4; i++ )
	{
		G_SoundIndex( va( "sound/chars/sand_creature/voice%d.mp3", i ) );
	}
	G_SoundIndex( "sound/chars/sand_creature/slither.wav" );
}

void SandCreature_ClearTimers( gentity_t *ent )
{
	TIMER_Set( NPCS.NPC, "speaking", -level.time );
	TIMER_Set( NPCS.NPC, "breaching", -level.time );
	TIMER_Set( NPCS.NPC, "breachDebounce", -level.time );
	TIMER_Set( NPCS.NPC, "pain", -level.time );
	TIMER_Set( NPCS.NPC, "attacking", -level.time );
	TIMER_Set( NPCS.NPC, "missDebounce", -level.time );
}

/* SP NUAM
void NPC_SandCreature_Die( gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc )
{
	//FIXME: somehow make him solid when he dies?
}
*/

void NPC_SandCreature_Pain( gentity_t *self, gentity_t *attacker, int damage )
//void NPC_SandCreature_Pain( gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc )
{
	int counter;
	if ( TIMER_Done( self, "pain" ) )
	{
		//float playerDist;
		vec3_t shakePos;
		//FIXME: effect and sound
		//FIXME: shootable during this anim?
		NPC_SetAnim( self, SETANIM_LEGS, Q_irand(BOTH_ATTACK1,BOTH_ATTACK2), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
		G_AddEvent( self, EV_PAIN, Q_irand( 0, 100 ) );
		TIMER_Set( self, "pain", self->client->ps.legsTimer + Q_irand( 500, 2000 ) );
		//wahoo fix for el camera shake effect
		G_GetBoltPosition( self, self->client->renderInfo.headBolt, shakePos, 0 );
		for(counter = 0; counter < MAX_CLIENTS; counter++)
		{
			float playerDist;
			gentity_t *radiusEnt = &g_entities[counter];
			if(radiusEnt && radiusEnt->client)
			{
				playerDist = Distance( radiusEnt->r.currentOrigin, self->r.currentOrigin );
				if ( playerDist < 256 )
					G_ScreenShake( shakePos, radiusEnt, 2.0f, 1000, qfalse );
			}
		}
	}
	self->enemy = self->NPC->goalEntity = NULL;
}

void SandCreature_MoveEffect( void )
{
	vec3_t	up = {0,0,1};
	vec3_t shakePos;
	vec3_t	org;
	int i;
	org[0] = NPCS.NPC->r.currentOrigin[0];
	org[1] = NPCS.NPC->r.currentOrigin[1];
	org[2] = NPCS.NPC->r.absmin[2]+2;

	//float playerDist = Distance( /*wahoo change*/NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin );
	//if ( playerDist < 256 )
	//{
		//CGCam_Shake( 0.75f*playerDist/256.0f, 250 );
		G_GetBoltPosition( NPCS.NPC, NPCS.NPC->client->renderInfo.headBolt, shakePos, 0 );
	//}
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		float playerDist;
		gentity_t *radiusEnt = &g_entities[i];
		if(radiusEnt && radiusEnt->client)
		{
			playerDist = Distance( radiusEnt->r.currentOrigin, NPCS.NPC->r.currentOrigin );
			if ( playerDist < 256 )
				G_ScreenShake( shakePos, radiusEnt, 2.0f, 250, qfalse );
		}
	}

	
	if ( level.time-NPCS.NPC->client->ps.legsTimer > 2000 )
	{//first time moving for at least 2 seconds
		//clear speakingtime
		TIMER_Set( NPCS.NPC, "speaking", -level.time );
	}

	if ( TIMER_Done( NPCS.NPC, "breaching" ) 
		&& TIMER_Done( NPCS.NPC, "breachDebounce" )
		&& TIMER_Done( NPCS.NPC, "pain" )
		&& TIMER_Done( NPCS.NPC, "attacking" )
		&& !Q_irand( 0, 10 ) )
	{//Breach!
		//FIXME: only do this while moving forward?
		trace_t	trace;
		//make him solid here so he can be hit/gets blocked on stuff. Check clear first.
		trap->Trace( &trace, NPCS.NPC->r.currentOrigin, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, NPCS.NPC->r.currentOrigin, NPCS.NPC->s.number, MASK_NPCSOLID, qfalse, 0, 0 );
		if ( !trace.allsolid && !trace.startsolid )
		{
			NPCS.NPC->clipmask = MASK_NPCSOLID;//turn solid for a little bit
			NPCS.NPC->r.contents = CONTENTS_BODY;
			//NPC->takedamage = qtrue;//can be shot?

			//FIXME: Breach sound?
			//FIXME: Breach effect?
			NPC_SetAnim( NPCS.NPC, SETANIM_LEGS, BOTH_WALK2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
			TIMER_Set( NPCS.NPC, "breaching", NPCS.NPC->client->ps.legsTimer );
			TIMER_Set( NPCS.NPC, "breachDebounce", NPCS.NPC->client->ps.legsTimer+Q_irand( 0, 10000 ) );
		}
	}
	if ( !TIMER_Done( NPCS.NPC, "breaching" ) )
	{//different effect when breaching
		//FIXME: make effect
		/*wahoo change to effect id*/G_PlayEffectID( G_EffectIndex( "env/sand_move_breach" ), org, up );
	}
	else
	{
		/*wahoo change to effect id*/G_PlayEffectID( G_EffectIndex( "env/sand_move" ), org, up );
	}
	NPCS.NPC->s.loopSound = G_SoundIndex( "sound/chars/sand_creature/slither.wav" );
}

qboolean SandCreature_CheckAhead( vec3_t end )
{
	float	radius;
	float	dist;
	float	tFrac; 
	trace_t	trace;
	int clipmask = NPCS.NPC->clipmask|CONTENTS_BOTCLIP;

	//make sure our goal isn't underground (else the trace will fail)
	vec3_t	bottom;
	bottom[0] = end[0];
	bottom[1] = end[1];
	bottom[2] = end[2]+NPCS.NPC->r.mins[2];
	trap->Trace( &trace, end, vec3_origin, vec3_origin, bottom, NPCS.NPC->s.number, NPCS.NPC->clipmask, qfalse, 0, 0 );
	if ( trace.fraction < 1.0f )
	{//in the ground, raise it up
		end[2] -= NPCS.NPC->r.mins[2]*(1.0f-trace.fraction)-0.125f;
	}

	trap->Trace( &trace, NPCS.NPC->r.currentOrigin, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, end, NPCS.NPC->s.number, clipmask, qfalse, 0, 0 );

	if ( trace.startsolid&&(trace.contents&CONTENTS_BOTCLIP) )
	{//started inside do not enter, so ignore them
		clipmask &= ~CONTENTS_BOTCLIP;
		trap->Trace( &trace, NPCS.NPC->r.currentOrigin, NPCS.NPC->r.mins, NPCS.NPC->r.maxs, end, NPCS.NPC->s.number, clipmask, qfalse, 0, 0 );
	}
	//Do a simple check
	if ( ( trace.allsolid == qfalse ) && ( trace.startsolid == qfalse ) && ( trace.fraction == 1.0f ) )
		return qtrue;

	if ( trace.plane.normal[2] >= .7f )
	{
		return qtrue;
	}

	//This is a work around
	radius = ( NPCS.NPC->r.maxs[0] > NPCS.NPC->r.maxs[1] ) ? NPCS.NPC->r.maxs[0] : NPCS.NPC->r.maxs[1];
	dist = Distance( NPCS.NPC->r.currentOrigin, end );
	tFrac = 1.0f - ( radius / dist );

	if ( trace.fraction >= tFrac )
		return qtrue;

	return qfalse;
}

qboolean SandCreature_Move( void )
{
	qboolean moved = qfalse;
	//FIXME should ignore doors..?
	vec3_t dest;
	VectorCopy( NPCS.NPCInfo->goalEntity->r.currentOrigin, dest );
	//Sand Creatures look silly using waypoints when they can go straight to the goal
	if ( SandCreature_CheckAhead( dest ) )
	{//use our temp move straight to goal check
		VectorSubtract( dest, NPCS.NPC->r.currentOrigin, NPCS.NPC->client->ps.moveDir );
		NPCS.NPC->client->ps.speed = VectorNormalize( NPCS.NPC->client->ps.moveDir );
		if ( (NPCS.ucmd.buttons&BUTTON_WALKING) && NPCS.NPC->client->ps.speed > NPCS.NPCInfo->stats.walkSpeed )
		{
			NPCS.NPC->client->ps.speed = NPCS.NPCInfo->stats.walkSpeed;
		}
		else
		{
			if ( NPCS.NPC->client->ps.speed < NPCS.NPCInfo->stats.walkSpeed )
			{
				NPCS.NPC->client->ps.speed = NPCS.NPCInfo->stats.walkSpeed;
			}
			if ( !(NPCS.ucmd.buttons&BUTTON_WALKING) && NPCS.NPC->client->ps.speed < NPCS.NPCInfo->stats.runSpeed )
			{
				NPCS.NPC->client->ps.speed = NPCS.NPCInfo->stats.runSpeed;
			}
			else if ( NPCS.NPC->client->ps.speed > NPCS.NPCInfo->stats.runSpeed )
			{
				NPCS.NPC->client->ps.speed = NPCS.NPCInfo->stats.runSpeed;
			}
		}
		moved = qtrue;
	}
	else
	{
		moved = NPC_MoveToGoal( qtrue );
	}
	if ( moved && NPCS.NPC->radius )
	{
		vec3_t	newPos;
		float curTurfRange, newTurfRange;
		curTurfRange = DistanceHorizontal( NPCS.NPC->r.currentOrigin, NPCS.NPC->s.origin );
		VectorMA( NPCS.NPC->r.currentOrigin, NPCS.NPC->client->ps.speed/100.0f, NPCS.NPC->client->ps.moveDir, newPos );
		newTurfRange = DistanceHorizontal( newPos, NPCS.NPC->s.origin );
		if ( newTurfRange > NPCS.NPC->radius && newTurfRange > curTurfRange )
		{//would leave our range
			//stop
			NPCS.NPC->client->ps.speed = 0.0f;
			VectorClear( NPCS.NPC->client->ps.moveDir );
			NPCS.ucmd.forwardmove = NPCS.ucmd.rightmove = 0;
			moved = qfalse;
		}
	}
	return (moved);
	//often erroneously returns false ???  something wrong with NAV...?
}

void SandCreature_Attack( qboolean miss )
{
	//float playerDist;
	vec3_t shakePos;
	int i;
	//FIXME: make it able to grab a thermal detonator, take it down, 
	//		then have it explode inside them, killing them 
	//		(or, do damage, making them stick half out of the ground and
	//		screech for a bit, giving you a chance to run for it!)

	//FIXME: effect and sound
	//FIXME: shootable during this anim?
	if ( !NPCS.NPC->enemy->client )
	{
		NPC_SetAnim( NPCS.NPC, SETANIM_LEGS, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
	}
	else
	{
		NPC_SetAnim( NPCS.NPC, SETANIM_LEGS, Q_irand( BOTH_ATTACK1, BOTH_ATTACK2 ), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_RESTART );
	}
	//don't do anything else while in this anim
	TIMER_Set( NPCS.NPC, "attacking", NPCS.NPC->client->ps.legsTimer );
	//playerDist = Distance( /*wahoo change player->r.currentOrigin*/NPC->enemy->r.currentOrigin, NPC->r.currentOrigin );
	//if ( playerDist < 256 )
	//{
		//FIXME: tone this down
		//CGCam_Shake( 0.75f*playerDist/128.0f, NPC->client->ps.legsTimer );
		//CGCam_Shake( 1.0f*playerDist/128.0f, self->client->ps.legsTimer );
		//CGCam_Shake( 0.75f*playerDist/256.0f, 250 );
		G_GetBoltPosition( NPCS.NPC, NPCS.NPC->client->renderInfo.headBolt, shakePos, 0 );
		//G_ScreenShake( shakePos, NULL, 1.0f, NPC->client->ps.legsTimer, qfalse );

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		float playerDist;
		gentity_t *radiusEnt = &g_entities[i];
		if(radiusEnt && radiusEnt->client)
		{
			playerDist = Distance( radiusEnt->r.currentOrigin, NPCS.NPC->r.currentOrigin );
			if ( playerDist < 256 )
				G_ScreenShake( shakePos, radiusEnt, 1.0f, NPCS.NPC->client->ps.legsTimer, qfalse );
		}
	}


	//} 

	if ( miss )
	{//purposely missed him, chance of knocking him down
		//FIXME: if, during the attack anim, I do end up catching him close to my mouth, then snatch him anyway...
		if ( NPCS.NPC->enemy && NPCS.NPC->enemy->client )
		{
			vec3_t dir2Enemy;
			VectorSubtract( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, dir2Enemy );
			if ( dir2Enemy[2] < 30 )
			{
				dir2Enemy[2] = 30;
			}
			if ( g_npcspskill.integer > 0 )
			{
				float enemyDist = VectorNormalize( dir2Enemy );
				//FIXME: tone this down, smaller radius
				if ( enemyDist < 200 && NPCS.NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{
					float throwStr = ((200-enemyDist)*0.4f)+20;
					if ( throwStr > 45 )
					{
						throwStr = 45;
					}
					G_Throw( NPCS.NPC->enemy, dir2Enemy, throwStr );
					if ( g_npcspskill.integer > 1 )
					{//knock them down, too
						if ( NPCS.NPC->enemy->health > 0 
							&& Q_flrand( 50, 150 ) > enemyDist )
						{//knock them down
							G_Knockdown( NPCS.NPC->enemy, NPCS.NPC, dir2Enemy, 300, qtrue );
							if ( NPCS.NPC->enemy->s.number < MAX_CLIENTS )
							{//make the player look up at me
								vec3_t vAng;
								vectoangles( dir2Enemy, vAng );
								VectorSet( vAng, AngleNormalize180(vAng[PITCH])*-1, NPCS.NPC->enemy->client->ps.viewangles[YAW], 0 );
								SetClientViewAngle( NPCS.NPC->enemy, vAng );
							}
						}
					}
				}
			}
		}
	}
	else
	{
		NPCS.NPC->enemy->activator = NPCS.NPC; // kind of dumb, but when we are locked to the Rancor, we are owned by it.
		NPCS.NPC->activator = NPCS.NPC->enemy;//remember him
		//this guy isn't going anywhere anymore
		NPCS.NPC->enemy->r.contents = 0;
		NPCS.NPC->enemy->clipmask = 0;

		if ( NPCS.NPC->activator->client )
		{
			//NPC->activator->client->ps.SaberDeactivateTrail(0);
			NPCS.NPC->client->ps.eFlags2 |= EF2_GENERIC_NPC_FLAG;//wahoo fix - to get it to stick in the mouth
			NPCS.NPC->activator->client->ps.eFlags2 |= EF2_HELD_BY_MONSTER;//EF_HELD_BY_SAND_CREATURE;//wahoo fix
			if ( NPCS.NPC->activator->health > 0 && NPCS.NPC->activator->client )
			{
				G_AddEvent( NPCS.NPC->activator, Q_irand(EV_DEATH1, EV_DEATH3), 0 );
				NPC_SetAnim( NPCS.NPC->activator, SETANIM_LEGS, BOTH_SWIM_IDLE1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				NPC_SetAnim( NPCS.NPC->activator, SETANIM_TORSO, BOTH_FALLDEATH1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
				TossClientItems( NPCS.NPC );
				if ( NPCS.NPC->activator->NPC )
				{//no more thinking for you
					NPCS.NPC->activator->NPC->nextBStateThink = Q3_INFINITE;
				}
			}
			/*
			if ( !NPC->activator->s.number )
			{
				cg.overrides.active |= (CG_OVERRIDE_3RD_PERSON_CDP|CG_OVERRIDE_3RD_PERSON_RNG);
				cg.overrides.thirdPersonCameraDamp = 0;
				cg.overrides.thirdPersonRange = 120;
			}
			*/
		}
		else
		{
			NPCS.NPC->client->ps.eFlags2 |= EF2_GENERIC_NPC_FLAG;//wahoo fix - to get it to stick in the mouth
			NPCS.NPC->activator->s.eFlags2 |= EF2_HELD_BY_MONSTER;//EF_HELD_BY_SAND_CREATURE;//wahoo fix
		}
	}
}

float SandCreature_EntScore( gentity_t *ent )
{
	float moveSpeed, dist;

	if ( ent->client )
	{
		moveSpeed = VectorLengthSquared( ent->client->ps.velocity );
	}
	else
	{
		moveSpeed = VectorLengthSquared( ent->s.pos.trDelta );
	}
	dist = DistanceSquared( NPCS.NPC->r.currentOrigin, ent->r.currentOrigin );
	return (moveSpeed-dist);
}

void SandCreature_SeekEnt( gentity_t *bestEnt, float score )
{
	NPCS.NPCInfo->enemyLastSeenTime = level.time;
	VectorCopy( bestEnt->r.currentOrigin, NPCS.NPCInfo->enemyLastSeenLocation );
	NPC_SetMoveGoal( NPCS.NPC, NPCS.NPCInfo->enemyLastSeenLocation, 0, qfalse, -1, NULL );
	if ( score > MIN_SCORE )
	{
		NPCS.NPC->enemy = bestEnt;
	}
}

void SandCreature_CheckMovingEnts( void )
{
	gentity_t	*radiusEnts[ 128 ];
	int			numEnts;
	const float	radius = NPCS.NPCInfo->stats.earshot;
	int			i;
	vec3_t		mins, maxs;
	int bestEnt = -1;
	float bestScore = 0;
	float checkScore;
	int			iradiusEnts[ 128 ];

	for ( i = 0; i < 3; i++ )
	{
		mins[i] = NPCS.NPC->r.currentOrigin[i] - radius;
		maxs[i] = NPCS.NPC->r.currentOrigin[i] + radius;
	}

	numEnts = trap->EntitiesInBox( mins, maxs, /*radiusEnts*/iradiusEnts, 128 );

	for ( i = 0; i < numEnts; i++ )
	{
		radiusEnts[ i ] = &g_entities[iradiusEnts[i]];
			/*continue;//wahoo fix*/

		if ( !radiusEnts[i]->inuse )
		{
			continue;
		}
		
		if ( radiusEnts[i] == NPCS.NPC )
		{//Skip the itself
			continue;
		}
		
		if ( radiusEnts[i]->client == NULL )
		{//must be a client
			if ( radiusEnts[i]->s.eType != ET_MISSILE
				|| radiusEnts[i]->s.weapon != WP_THERMAL )
			{//not a thermal detonator
				continue;
			}
		}
		else
		{
			if ( (radiusEnts[i]->client->ps.eFlags&EF2_HELD_BY_MONSTER) )
			{//can't be one being held
				continue;
			}

			/*if ( (radiusEnts[i]->client->ps.eFlags&EF_HELD_BY_WAMPA) )
			{//can't be one being held
				continue;
			}

			if ( (radiusEnts[i]->client->ps.eFlags&EF_HELD_BY_SAND_CREATURE) )
			{//can't be one being held
				continue;
			}*/

			if ( (radiusEnts[i]->s.eFlags&EF_NODRAW) )
			{//not if invisible
				continue;
			}

			if ( radiusEnts[i]->client->ps.groundEntityNum != ENTITYNUM_WORLD )
			{//not on the ground
				continue;
			}

			if ( radiusEnts[i]->client->NPC_class == CLASS_SAND_CREATURE )
			{
				continue;
			}
		}

		if ( (radiusEnts[i]->flags&FL_NOTARGET) )
		{
			continue;
		}
		/*
		if ( radiusEnts[i]->client && (radiusEnts[i]->client->NPC_class == CLASS_RANCOR || radiusEnts[i]->client->NPC_class == CLASS_ATST ) )
		{//can't grab rancors or atst's
			continue;
		}
		*/
		checkScore = SandCreature_EntScore( radiusEnts[i] );
		//FIXME: take mass into account too?  What else?
		if ( checkScore > bestScore )
		{
			bestScore = checkScore;
			bestEnt = i;
		}
	}
	if ( bestEnt != -1 )
	{
		SandCreature_SeekEnt( radiusEnts[bestEnt], bestScore );
	}
}

void SandCreature_SeekAlert( int alertEvent )
{
	alertEvent_t *alert = &level.alertEvents[alertEvent];

	//FIXME: check for higher alert status or closer than last location?
	NPCS.NPCInfo->enemyLastSeenTime = level.time;
	VectorCopy( alert->position, NPCS.NPCInfo->enemyLastSeenLocation );
	NPC_SetMoveGoal( NPCS.NPC, NPCS.NPCInfo->enemyLastSeenLocation, 0, qfalse, -1, NULL );
}

void SandCreature_CheckAlerts( void )
{
	if ( !(NPCS.NPCInfo->scriptFlags&SCF_IGNORE_ALERTS) )
	{
		int alertEvent = NPC_CheckAlertEvents( qfalse, qtrue, NPCS.NPCInfo->lastAlertID, qfalse, AEL_MINOR, qtrue );

		//There is an event to look at
		if ( alertEvent >= 0 )
		{
			//if ( level.alertEvents[alertEvent].ID != NPCInfo->lastAlertID )
			{
				SandCreature_SeekAlert( alertEvent );
			}
		}
	}
}

float SandCreature_DistSqToGoal( qboolean goalIsEnemy )
{
	float goalDistSq;
	if ( !NPCS.NPCInfo->goalEntity || goalIsEnemy )
	{
		if ( !NPCS.NPC->enemy )
		{
			return Q3_INFINITE;
		}
		NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
	}

	if ( NPCS.NPCInfo->goalEntity->client )
	{
		goalDistSq = DistanceSquared( NPCS.NPC->r.currentOrigin, NPCS.NPCInfo->goalEntity->r.currentOrigin );
	}
	else
	{
		vec3_t gOrg;
		VectorCopy( NPCS.NPCInfo->goalEntity->r.currentOrigin, gOrg );
		gOrg[2] -= (NPCS.NPC->r.mins[2]-NPCS.NPCInfo->goalEntity->r.mins[2]);//moves the gOrg up/down to make it's origin seem at the proper height as if it had my mins
		goalDistSq = DistanceSquared( NPCS.NPC->r.currentOrigin, gOrg );
	}
	return goalDistSq;
}

void SandCreature_Chase( void )
{
	float enemyDistSq = SandCreature_DistSqToGoal( qtrue );

	if ( !NPCS.NPC->enemy->inuse )
	{//freed
		NPCS.NPC->enemy = NULL;
		return;
	}
	
	if ( /*wahoo change  r.svFlags&SVF_LOCKEDENEMY*/(NPCS.NPC->NPC->aiFlags&NPCAI_LOCKEDENEMY) )
	{//always know where he is
		NPCS.NPCInfo->enemyLastSeenTime = level.time;
	}

	if ( /*wahoo change  r.svFlags&SVF_LOCKEDENEMY*/!(NPCS.NPC->NPC->aiFlags&NPCAI_LOCKEDENEMY) )
	{
		if ( level.time-NPCS.NPCInfo->enemyLastSeenTime > 10000 )
		{
			NPCS.NPC->enemy = NULL;
			return;
		}
	}

	if ( NPCS.NPC->enemy->client )
	{
		if ( NPCS.NPC->enemy->client->ps.eFlags&EF2_HELD_BY_MONSTER)
			//|| (NPC->enemy->client->ps.eFlags&EF_HELD_BY_RANCOR)
			//|| (NPC->enemy->client->ps.eFlags&EF_HELD_BY_WAMPA) )
		{//was picked up by another monster, forget about him
			NPCS.NPC->enemy = NULL;
			//wahoo changeNPC->r.svFlags &= ~SVF_LOCKEDENEMY;
			NPCS.NPC->NPC->aiFlags &= ~NPCAI_LOCKEDENEMY;
			return;
		}
	}
	//chase the enemy
	if ( NPCS.NPC->enemy->client 
		&& NPCS.NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_WORLD 
		&& !(/*NPC->r.svFlags&SVF_LOCKEDENEMY wahoo fix*/NPCS.NPC->NPC->aiFlags&NPCAI_LOCKEDENEMY) )
	{//off the ground!
		//FIXME: keep moving in the dir we were moving for a little bit...
	}
	else
	{
		float enemyScore = SandCreature_EntScore( NPCS.NPC->enemy );
		if ( enemyScore < MIN_SCORE 
			&& !(/*NPC->r.svFlags&SVF_LOCKEDENEMY wahoo fix*/NPCS.NPC->NPC->aiFlags&NPCAI_LOCKEDENEMY) )
		{//too slow or too far away
		}
		else
		{
			float moveSpeed;
			if ( NPCS.NPC->enemy->client )
			{
				moveSpeed = VectorLengthSquared( NPCS.NPC->enemy->client->ps.velocity );
			}
			else
			{
				moveSpeed = VectorLengthSquared( NPCS.NPC->enemy->s.pos.trDelta );
			}
			if ( moveSpeed )
			{//he's still moving, update my goalEntity's origin
				SandCreature_SeekEnt( NPCS.NPC->enemy, 0 );
				NPCS.NPCInfo->enemyLastSeenTime = level.time;
			}
		}
	}

	if ( (level.time-NPCS.NPCInfo->enemyLastSeenTime) > 5000 
		&& !(/*NPC->r.svFlags&SVF_LOCKEDENEMY wahoo fix*/NPCS.NPC->NPC->aiFlags&NPCAI_LOCKEDENEMY) )
	{//enemy hasn't moved in about 5 seconds, see if there's anything else of interest
		SandCreature_CheckAlerts();
		SandCreature_CheckMovingEnts();
	}



	//FIXME: keeps chasing goalEntity even when it's already reached it...?
	if ( enemyDistSq >= MIN_ATTACK_DIST_SQ//NPCS.NPCInfo->goalEntity &&
		&& (level.time-NPCS.NPCInfo->enemyLastSeenTime) <= 3000 )
	{//sensed enemy (or something) less than 3 seconds ago
		NPCS.ucmd.buttons &= ~BUTTON_WALKING;
		if ( SandCreature_Move() )
		{
			SandCreature_MoveEffect();
		}
	}
	else if ( (level.time-NPCS.NPCInfo->enemyLastSeenTime) <= 5000 
		&& !(/*NPC->r.svFlags&SVF_LOCKEDENEMY wahoo fix*/NPCS.NPC->NPC->aiFlags&NPCAI_LOCKEDENEMY) )
	{//NOTE: this leaves a 2-second dead zone in which they'll just sit there unless their enemy moves
		//If there is an event we might be interested in if we weren't still interested in our enemy
		if ( NPC_CheckAlertEvents( qfalse, qtrue, NPCS.NPCInfo->lastAlertID, qfalse, AEL_MINOR, qtrue ) >= 0 )
		{//just stir
			SandCreature_MoveEffect();
		}
	}

	if ( enemyDistSq < MIN_ATTACK_DIST_SQ )
	{
		if ( NPCS.NPC->enemy->client )
		{
			NPCS.NPC->client->ps.viewangles[YAW] = NPCS.NPC->enemy->client->ps.viewangles[YAW];
		}
		if ( TIMER_Done( NPCS.NPC, "breaching" ) )
		{
			//okay to attack
			SandCreature_Attack( qfalse );
		}
	}
	else if ( enemyDistSq < MAX_MISS_DIST_SQ 
		&& enemyDistSq > MIN_MISS_DIST_SQ
		&& NPCS.NPC->enemy->client 
		&& TIMER_Done( NPCS.NPC, "breaching" )
		&& TIMER_Done( NPCS.NPC, "missDebounce" )
		&& !VectorCompare( NPCS.NPC->pos1, NPCS.NPC->r.currentOrigin ) //so we don't come up again in the same spot
		&& !Q_irand( 0, 10 ) )
	{
		if ( !(/*NPC->r.svFlags&SVF_LOCKEDENEMY wahoo fix*/NPCS.NPC->NPC->aiFlags&NPCAI_LOCKEDENEMY) )
		{
			//miss them
			SandCreature_Attack( qtrue );
			VectorCopy( NPCS.NPC->r.currentOrigin, NPCS.NPC->pos1 );
			TIMER_Set( NPCS.NPC, "missDebounce", Q_irand( 3000, 10000 ) );
		}
	}
}

void SandCreature_Hunt( void )
{
	SandCreature_CheckAlerts();
	SandCreature_CheckMovingEnts();
	//If we have somewhere to go, then do that
	//FIXME: keeps chasing goalEntity even when it's already reached it...?
	if ( NPCS.NPCInfo->goalEntity 
		&& SandCreature_DistSqToGoal( qfalse ) >= MIN_ATTACK_DIST_SQ )
	{
		NPCS.ucmd.buttons |= BUTTON_WALKING;
		if ( SandCreature_Move() )
		{
			SandCreature_MoveEffect();
		}
	}
	else
	{
		NPC_ReachedGoal();
	}
}

void SandCreature_Sleep( void )
{
	SandCreature_CheckAlerts();
	SandCreature_CheckMovingEnts();
	//FIXME: keeps chasing goalEntity even when it's already reached it!
	if ( NPCS.NPCInfo->goalEntity 
		&& SandCreature_DistSqToGoal( qfalse ) >= MIN_ATTACK_DIST_SQ )
	{
		NPCS.ucmd.buttons |= BUTTON_WALKING;
		if ( SandCreature_Move() )
		{
			SandCreature_MoveEffect();
		}
	}
	else
	{
		NPC_ReachedGoal();
	}
	/*
	if ( UpdateGoal() )
	{
		ucmd.buttons |= BUTTON_WALKING;
		//FIXME: Sand Creatures look silly using waypoints when they can go straight to the goal
		if ( SandCreature_Move() )
		{
			SandCreature_MoveEffect();
		}
	}
	*/
}

void	SandCreature_PushEnts()
{
  	int			numEnts;
	gentity_t*	radiusEnts[128];
	const float	radius = 70;
	vec3_t		mins, maxs;
	vec3_t		smackDir;
	float		smackDist;
	int			i, entIndex;
	int			iradiusEnts[ 128 ];

	for (i = 0; i < 3; i++ )
	{
		mins[i] = NPCS.NPC->r.currentOrigin[i] - radius;
		maxs[i] = NPCS.NPC->r.currentOrigin[i] + radius;
	}

	numEnts = trap->EntitiesInBox(mins, maxs, /*radiusEnts*/iradiusEnts, 128);
	for (entIndex=0; entIndex<numEnts; entIndex++)
	{
		radiusEnts[ entIndex ] = &g_entities[iradiusEnts[entIndex]];
			/*continue;//wahoo fix*/

		// Only Clients
		//--------------
		if (!radiusEnts[entIndex] || !radiusEnts[entIndex]->client || radiusEnts[entIndex]==NPCS.NPC)
		{
			continue;
		}

		// Do The Vector Distance Test
		//-----------------------------
		VectorSubtract(radiusEnts[entIndex]->r.currentOrigin, NPCS.NPC->r.currentOrigin, smackDir);
		smackDist = VectorNormalize(smackDir);
		if (smackDist<radius)
		{
 			G_Throw(radiusEnts[entIndex], smackDir, 90);
		}
	}
}

void NPC_BSSandCreature_Default( void )
{
	qboolean visible = qfalse;
	
	//clear it every frame, will be set if we actually move this frame...
	NPCS.NPC->s.loopSound = 0;

	if ( NPCS.NPC->health > 0 && TIMER_Done( NPCS.NPC, "breaching" ) )
	{//go back to non-solid mode
		if ( NPCS.NPC->r.contents )
		{
			NPCS.NPC->r.contents = 0;
		}
		if ( NPCS.NPC->clipmask == MASK_NPCSOLID )
		{
			NPCS.NPC->clipmask = CONTENTS_SOLID|CONTENTS_MONSTERCLIP;
		}
		if ( TIMER_Done( NPCS.NPC, "speaking" ) )
		{
			G_SoundOnEnt( NPCS.NPC, CHAN_VOICE, va( "sound/chars/sand_creature/voice%d.mp3", Q_irand( 1, 3 ) ) );
			TIMER_Set( NPCS.NPC, "speaking", Q_irand( 3000, 10000 ) );
		}
	}
	else
	{//still in breaching anim
		visible = qtrue;
		//FIXME: maybe push things up/away and maybe knock people down when doing this?
		//FIXME: don't turn while breaching?
		//FIXME: move faster while breaching?
		//NOTE: shaking now done whenever he moves
	}

	//FIXME: when in start and end of attack/pain anims, need ground disturbance effect around him
	// NOTENOTE: someone stubbed this code in, so I figured I'd use it.  The timers are all weird, ie, magic numbers that sort of work, 
	//	but maybe I'll try and figure out real values later if I have time.
	if ( NPCS.NPC->client->ps.legsAnim == BOTH_ATTACK1
		|| NPCS.NPC->client->ps.legsAnim == BOTH_ATTACK2 ) 
	{//FIXME: get start and end frame numbers for this effect for each of these anims
		vec3_t	up={0,0,1};
		vec3_t org;
		VectorCopy( NPCS.NPC->r.currentOrigin, org );
		org[2] -= 40; 
		if ( NPCS.NPC->client->ps.legsTimer > 3700 )
		{
//			/*wahoo change to effect id*/G_PlayEffectID( G_EffectIndex( "env/sand_dive"  ), NPC->r.currentOrigin, up );
			/*wahoo change to effect id*/G_PlayEffectID( G_EffectIndex( "env/sand_spray"  ), org, up );
		}
		else if ( NPCS.NPC->client->ps.legsTimer > 1600 	&& NPCS.NPC->client->ps.legsTimer < 1900 )
		{
			/*wahoo change to effect id*/G_PlayEffectID( G_EffectIndex( "env/sand_spray"  ), org, up );
		}
		///*wahoo change to effect id*/G_PlayEffectID( G_EffectIndex( "env/sand_attack_breach" ), org, up );
	}
	

	if ( !TIMER_Done( NPCS.NPC, "pain" ) )
	{
		visible = qtrue;
	}
	else if ( !TIMER_Done( NPCS.NPC, "attacking" ) )
	{
		visible = qtrue;
	}
	else
	{
		if ( NPCS.NPC->activator )
		{//kill and remove the guy we ate
			//FIXME: want to play ...?  What was I going to say?
			G_Damage(NPCS.NPC->activator, NPCS.NPC, NPCS.NPC, NULL, NPCS.NPC->activator->s.origin, NPCS.NPC->activator->health*2, DAMAGE_NO_PROTECTION|DAMAGE_NO_KNOCKBACK, MOD_MELEE);
			if ( NPCS.NPC->activator->client )
			{//racc - picked up an NPC or client, make them non-visible and then drop them.
				NPCS.NPC->client->ps.eFlags |= EF_NODRAW;//wahoofix - fix so that the person doesn't just jump out of the game entirely
				Rancor_DropVictim(NPCS.NPC);//wahoo - drop the dude after you go back down
			}

			NPCS.NPC->activator = NPCS.NPC->enemy = NPCS.NPCInfo->goalEntity = NULL;
		}

		if ( NPCS.NPC->enemy )
		{
			SandCreature_Chase();
		}
		else if ( (level.time - NPCS.NPCInfo->enemyLastSeenTime) < 5000 )//FIXME: should make this able to be variable
		{//we were alerted recently, move towards there and look for footsteps, etc.
			SandCreature_Hunt();
		}
		else
		{//no alerts, sleep and wake up only by alerts
			//FIXME: keeps chasing goalEntity even when it's already reached it!
			SandCreature_Sleep();
		}
	}
	NPC_UpdateAngles( qtrue, qtrue );
	if ( !visible )
	{
		NPCS.NPC->client->ps.eFlags |= EF_NODRAW;
		NPCS.NPC->s.eFlags |= EF_NODRAW;
	}
	else
	{
		NPCS.NPC->client->ps.eFlags &= ~EF_NODRAW;
		NPCS.NPC->s.eFlags &= ~EF_NODRAW;

		SandCreature_PushEnts();
	}
}

//FIXME: need pain behavior of sticking up through ground, writhing and screaming
//FIXME: need death anim like pain, but flopping aside and staying above ground...
//[/NPCSandCreature]

