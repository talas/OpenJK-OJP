/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2003 - 2008, OJP contributors
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
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

#include "b_local.h"

// These define the working combat range for these suckers
#define MIN_DISTANCE		54
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define MAX_DISTANCE		128
#define MAX_DISTANCE_SQR	( MAX_DISTANCE * MAX_DISTANCE )

#define LSTATE_CLEAR		0
#define LSTATE_WAITING		1
//[CoOp]
#define LSTATE_FLEE			2
#define LSTATE_BERZERK		3

#define HOWLER_RETREAT_DIST	300.0f
#define HOWLER_PANIC_HEALTH	10
//[/CoOp]

//[CoOp]
static void Howler_Attack( float enemyDist, qboolean howl );
extern qboolean NPC_TryJump_Gent(gentity_t *goal,	float max_xy_dist, float max_z_diff);
extern void G_GetBoltPosition( gentity_t *self, int boltIndex, vec3_t pos, int modelIndex ); //NPC_utils.c
extern qboolean NAV_DirSafe( gentity_t *self, vec3_t dir, float dist );
//[/CoOp]

/*
-------------------------
NPC_Howler_Precache
-------------------------
*/
void NPC_Howler_Precache( void )
{
	//[CoOp]
	//added howler precashe from SP
	int i;
	G_EffectIndex( "howler/sonic" );
	G_SoundIndex( "sound/chars/howler/howl.mp3" );
	for ( i = 1; i < 3; i++ )
	{
		G_SoundIndex( va( "sound/chars/howler/idle_hiss%d.mp3", i ) );
	}
	for ( i = 1; i < 6; i++ )
	{
		G_SoundIndex( va( "sound/chars/howler/howl_talk%d.mp3", i ) );
		G_SoundIndex( va( "sound/chars/howler/howl_yell%d.mp3", i ) );
	}
	//[/CoOp]
}

//[CoOp]
//added from SP code
void Howler_ClearTimers( gentity_t *self )
{
	//clear all my timers
	TIMER_Set( self, "flee", -level.time );
	TIMER_Set( self, "retreating", -level.time );
	TIMER_Set( self, "standing", -level.time );
	TIMER_Set( self, "walking", -level.time );
	TIMER_Set( self, "running", -level.time );
	TIMER_Set( self, "aggressionDecay", -level.time );
	TIMER_Set( self, "speaking", -level.time );
}


//added from SP
static qboolean NPC_Howler_Move( int randomJumpChance )
{
	if ( !TIMER_Done( NPCS.NPC, "standing" ) )
	{//standing around
		return qfalse;
	}
	if ( NPCS.NPC->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//in air, don't do anything
		return qfalse;
	}
	if ( (!NPCS.NPC->enemy&&TIMER_Done( NPCS.NPC, "running" )) || !TIMER_Done( NPCS.NPC, "walking" ) )
	{
		NPCS.ucmd.buttons |= BUTTON_WALKING;
	}
	if ( (!randomJumpChance||Q_irand( 0, randomJumpChance ))
		&& NPC_MoveToGoal( qtrue ) )
	{
		if ( VectorCompare( NPCS.NPC->client->ps.moveDir, vec3_origin )
			|| !NPCS.NPC->client->ps.speed )
		{//uh.... wtf?  Got there?
			if ( NPCS.NPCInfo->goalEntity )
			{
				NPC_FaceEntity( NPCS.NPCInfo->goalEntity, qfalse );
			}
			else
			{
				NPC_UpdateAngles( qfalse, qtrue );
			}
			return qtrue;
		}
		//TEMP: don't want to strafe
		VectorClear( NPCS.NPC->client->ps.moveDir );
		NPCS.ucmd.rightmove = 0.0f;
//		Com_Printf( "Howler moving %d\n",NPCS.ucmd.forwardmove );
		//if backing up, go slow...
		if ( NPCS.ucmd.forwardmove < 0.0f )
		{
			NPCS.ucmd.buttons |= BUTTON_WALKING;
			//if ( NPCS.NPC->client->ps.speed > NPCS.NPCInfo->stats.walkSpeed )
			{//don't walk faster than I'm allowed to
				NPCS.NPC->client->ps.speed = NPCS.NPCInfo->stats.walkSpeed;
			}
		}
		else
		{
			if ( (NPCS.ucmd.buttons&BUTTON_WALKING) )
			{
				NPCS.NPC->client->ps.speed = NPCS.NPCInfo->stats.walkSpeed;
			}
			else
			{
				NPCS.NPC->client->ps.speed = NPCS.NPCInfo->stats.runSpeed;
			}
		}
		NPCS.NPCInfo->lockedDesiredYaw = NPCS.NPCInfo->desiredYaw = NPCS.NPCInfo->lastPathAngles[YAW];
		NPC_UpdateAngles( qfalse, qtrue );
	}
	else if ( NPCS.NPCInfo->goalEntity )
	{//couldn't get where we wanted to go, try to jump there
		NPC_FaceEntity( NPCS.NPCInfo->goalEntity, qfalse );
		NPC_TryJump_Gent( NPCS.NPCInfo->goalEntity, 400.0f, -256.0f );
	}
	return qtrue;
}
//[/CoOp]


/*
-------------------------
Howler_Idle
-------------------------
*/
void Howler_Idle( void ) {
}


/*
-------------------------
Howler_Patrol
-------------------------
*/
//[CoOp]
//Replaced with SP version
static void Howler_Patrol( void )
{
	gentity_t *ClosestPlayer = FindClosestPlayer(NPCS.NPC->r.currentOrigin, NPCS.NPC->client->enemyTeam);

	NPCS.NPCInfo->localState = LSTATE_CLEAR;

	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		NPC_Howler_Move( 100 );
	}

	if(ClosestPlayer)
	{//attack enemy players that are close.
		if(Distance(ClosestPlayer->r.currentOrigin, NPCS.NPC->r.currentOrigin) < 256 * 256)
		{
			G_SetEnemy( NPCS.NPC, ClosestPlayer );
		}
	}

	if ( NPC_CheckEnemyExt( qtrue ) == qfalse )
	{
		Howler_Idle();
		return;
	}

	Howler_Attack( 0.0f, qtrue );
}
 
/*
-------------------------
Howler_Move
-------------------------
*/
//replaced with SP version
static qboolean Howler_Move( qboolean visible )
{
	if ( NPCS.NPCInfo->localState != LSTATE_WAITING )
	{
		NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
		NPCS.NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range
		return NPC_Howler_Move( 30 );
	}
	return qfalse;
}

//---------------------------------------------------------
//replaced with SP version
extern qboolean PM_InKnockDown( playerState_t *ps );
//[KnockdownSys]
extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
//extern void G_Knockdown( gentity_t *victim );
//[/KnockdownSys]
static void Howler_TryDamage( int damage, qboolean tongue, qboolean knockdown )
{
	vec3_t	start, end, dir;
	trace_t	tr;
	float dist;

	if ( tongue )
	{
		G_GetBoltPosition( NPCS.NPC, NPCS.NPC->NPC->genericBolt1, start, 0 );
		G_GetBoltPosition( NPCS.NPC, NPCS.NPC->NPC->genericBolt2, end, 0 );
		VectorSubtract( end, start, dir );
		dist = VectorNormalize( dir );
		VectorMA( start, dist+16, dir, end );
	}
	else
	{
		VectorCopy( NPCS.NPC->r.currentOrigin, start );
		AngleVectors( NPCS.NPC->r.currentAngles, dir, NULL, NULL );
		VectorMA( start, MIN_DISTANCE*2, dir, end );
	}
/* RACC - don't need this for now.
#ifndef FINAL_BUILD
	if ( d_saberCombat.integer > 1 )
	{
		G_DebugLine(start, end, 1000, 0x000000ff, qtrue);
	}
#endif
*/
	// Should probably trace from the mouth, but, ah well.
	trap->Trace( &tr, start, vec3_origin, vec3_origin, end, NPCS.NPC->s.number, MASK_SHOT, qfalse, 0, 0 );

	if ( tr.entityNum < ENTITYNUM_WORLD )
	{//hit *something*
		gentity_t *victim = &g_entities[tr.entityNum];
		if ( !victim->client
			|| victim->client->NPC_class != CLASS_HOWLER )
		{//not another howler

			if ( knockdown && victim->client )
			{//only do damage if victim isn't knocked down.  If he isn't, knock him down
				if ( PM_InKnockDown( &victim->client->ps ) )
				{
					return;
				}
			}

			//FIXME: some sort of damage effect (claws and tongue are cutting you... blood?)
			G_Damage( victim, NPCS.NPC, NPCS.NPC, dir, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
			if ( knockdown && victim->health > 0 )
			{//victim still alive
				//[KnockdownSys]
				G_Knockdown( victim, NPCS.NPC, NPCS.NPC->client->ps.velocity, 500, qfalse );
				//G_Knockdown(victim);
				//[/KnockdownSys]
			}
		}
	}
}



//Moved in from SP
extern int NPC_GetEntsNearBolt( int *radiusEnts, float radius, int boltIndex, vec3_t boltOrg );
extern float NPC_EntRangeFromBolt( gentity_t *targEnt, int boltIndex );
static void Howler_Howl( void )
{
	//gentity_t	*radiusEnts[ 128 ];
	int			radiusEntsNums[128];
	gentity_t	*ent;
	int			numEnts;
	const float	radius = (NPCS.NPC->spawnflags&1)?256:128;
	const float	halfRadSquared = ((radius/2)*(radius/2));
	const float	radiusSquared = (radius*radius);
	float		distSq;
	int			i;
	vec3_t		boltOrg;

	AddSoundEvent( NPCS.NPC, NPCS.NPC->r.currentOrigin, 512, AEL_DANGER, qfalse, qtrue );

	//RACC - handLBolt never defined for Howlers? *shrug* just use the tongue base
	numEnts = NPC_GetEntsNearBolt( radiusEntsNums, radius, NPCS.NPC->NPC->genericBolt1, boltOrg );
	//numEnts = NPC_GetEntsNearBolt( radiusEnts, radius, NPCS.NPC->handLBolt, boltOrg );

	for ( i = 0; i < numEnts; i++ )
	{
		ent = &g_entities[radiusEntsNums[i]];

		if ( !ent->inuse )
		{
			continue;
		}
		
		if ( ent == NPCS.NPC )
		{//Skip the rancor ent
			//RACC - assume this means the source howler
			continue;
		}
		
		if ( ent->client == NULL )
		{//must be a client
			continue;
		}

		if ( ent->client->NPC_class == CLASS_HOWLER )
		{//other howlers immune
			continue;
		}

		distSq = DistanceSquared( ent->r.currentOrigin, boltOrg );
		if ( distSq <= radiusSquared )
		{
			if ( distSq < halfRadSquared )
			{//close enough to do damage, too
				if ( Q_irand( 0, g_npcspskill.integer ) )
				{//does no damage on easy, does 1 point every other frame on medium, more often on hard
					//RACC - Impact isn't a MOD in MP.
					G_Damage( ent, NPCS.NPC, NPCS.NPC, vec3_origin, NPCS.NPC->r.currentOrigin, 1, DAMAGE_NO_KNOCKBACK, MOD_CRUSH );
					//G_Damage( ent, NPC, NPC, vec3_origin, NPC->r.currentOrigin, 1, DAMAGE_NO_KNOCKBACK, MOD_IMPACT );
				}
			}
			if ( ent->health > 0 
				&& ent->client
				&& ent->client->NPC_class != CLASS_RANCOR
				&& ent->client->NPC_class != CLASS_ATST 
				&& !PM_InKnockDown( &ent->client->ps ) )
			{
				if ( BG_HasAnimation( ent->localAnimIndex, BOTH_SONICPAIN_START ) )
				{
					if ( ent->client->ps.torsoAnim != BOTH_SONICPAIN_START 
						&& ent->client->ps.torsoAnim != BOTH_SONICPAIN_HOLD )
					{
						NPC_SetAnim( ent, SETANIM_LEGS, BOTH_SONICPAIN_START, SETANIM_FLAG_NORMAL );
						NPC_SetAnim( ent, SETANIM_TORSO, BOTH_SONICPAIN_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						ent->client->ps.torsoTimer += 100;
						ent->client->ps.weaponTime = ent->client->ps.torsoTimer;
					}
					else if ( ent->client->ps.torsoTimer <= 100 )
					{//at the end of the sonic pain start or hold anim
						NPC_SetAnim( ent, SETANIM_LEGS, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_NORMAL );
						NPC_SetAnim( ent, SETANIM_TORSO, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						ent->client->ps.torsoTimer += 100; 
						ent->client->ps.weaponTime = ent->client->ps.torsoTimer;
					}
				}
				/*  racc - commented out of SP code?
				else if ( distSq < halfRadSquared 
					&& radiusEnts[i]->client->ps.groundEntityNum != ENTITYNUM_NONE 
					&& !Q_irand( 0, 10 ) )//FIXME: base on skill
				{//within range
					G_Knockdown( radiusEnts[i], NPCS.NPC, vec3_origin, 500, qfalse );
				}
				*/
			}
		}

		//camera shaking
		distSq = Distance( boltOrg, ent->r.currentOrigin );
		if ( distSq < 256.0f )
		{
			G_ScreenShake(boltOrg, ent, 1.0f*distSq/128.0f, 200, qfalse);
		}
	}

	//RACC - changed to work for multiple players
	/*
	playerDist = NPC_EntRangeFromBolt( player, NPCS.NPC->genericBolt1 );
	if ( playerDist < 256.0f )
	{
		CGCam_Shake( 1.0f*playerDist/128.0f, 200 );
	}
	*/
}

//------------------------------
//replaced with SP version
void G_SoundOnEnt( gentity_t *ent, int channel, const char *soundPath );
static void Howler_Attack( float enemyDist, qboolean howl )
{
	int dmg = (NPCS.NPCInfo->localState==LSTATE_BERZERK)?5:2;

	vec3_t boltOrg;
	vec3_t fwd;

	if ( !TIMER_Exists( NPCS.NPC, "attacking" ))
	{
		int attackAnim = BOTH_GESTURE1;
		// Going to do an attack
		if ( NPCS.NPC->enemy && NPCS.NPC->enemy->client && PM_InKnockDown( &NPCS.NPC->enemy->client->ps )
			&& enemyDist <= MIN_DISTANCE )
		{
			attackAnim = BOTH_ATTACK2;
		}
		else if ( !Q_irand( 0, 4 ) || howl )
		{//howl attack
			//G_SoundOnEnt( NPCS.NPC, CHAN_VOICE, "sound/chars/howler/howl.mp3" );
		}
		else if ( enemyDist > MIN_DISTANCE && Q_irand( 0, 1 ) )
		{//lunge attack
			//jump foward
			vec3_t	fwd, yawAng;
			
			VectorSet( yawAng, 0, NPCS.NPC->client->ps.viewangles[YAW], 0 );
			AngleVectors( yawAng, fwd, NULL, NULL );
			VectorScale( fwd, (enemyDist*3.0f), NPCS.NPC->client->ps.velocity );
			NPCS.NPC->client->ps.velocity[2] = 200;
			NPCS.NPC->client->ps.groundEntityNum = ENTITYNUM_NONE;
			
			attackAnim = BOTH_ATTACK1;
		}
		else
		{//tongue attack
			attackAnim = BOTH_ATTACK2;
		}

		NPC_SetAnim( NPCS.NPC, SETANIM_BOTH, attackAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART );
		if ( NPCS.NPCInfo->localState == LSTATE_BERZERK )
		{//attack again right away
			TIMER_Set( NPCS.NPC, "attacking", NPCS.NPC->client->ps.legsTimer );
		}
		else
		{
			TIMER_Set( NPCS.NPC, "attacking", NPCS.NPC->client->ps.legsTimer + Q_irand( 0, 1500 ) );//FIXME: base on skill
			TIMER_Set( NPCS.NPC, "standing", -level.time );
			TIMER_Set( NPCS.NPC, "walking", -level.time );
			TIMER_Set( NPCS.NPC, "running", NPCS.NPC->client->ps.legsTimer + 5000 );
		}

		TIMER_Set( NPCS.NPC, "attack_dmg", 200 ); // level two damage
	}

	// Need to do delayed damage since the attack animations encapsulate multiple mini-attacks
	switch ( NPCS.NPC->client->ps.legsAnim )
	{
	case BOTH_ATTACK1:
	case BOTH_MELEE1:
		if ( NPCS.NPC->client->ps.legsTimer > 650//more than 13 frames left
			&& BG_AnimLength( NPCS.NPC->localAnimIndex, (animNumber_t)NPCS.NPC->client->ps.legsAnim ) - NPCS.NPC->client->ps.legsTimer >= 800 )//at least 16 frames into anim
		{
			Howler_TryDamage( dmg, qfalse, qfalse );
		}
		break;
	case BOTH_ATTACK2:
	case BOTH_MELEE2:
		if ( NPCS.NPC->client->ps.legsTimer > 350//more than 7 frames left
			&& BG_AnimLength( NPCS.NPC->localAnimIndex, (animNumber_t)NPCS.NPC->client->ps.legsAnim ) - NPCS.NPC->client->ps.legsTimer >= 550 )//at least 11 frames into anim
		{
			Howler_TryDamage( dmg, qtrue, qfalse );
		}
		break;
	case BOTH_GESTURE1:
		{
			if ( NPCS.NPC->client->ps.legsTimer > 1800//more than 36 frames left
				&& BG_AnimLength( NPCS.NPC->localAnimIndex, (animNumber_t)NPCS.NPC->client->ps.legsAnim ) - NPCS.NPC->client->ps.legsTimer >= 950 )//at least 19 frames into anim
			{
				Howler_Howl();
				if ( !NPCS.NPC->count )
				{
					//RAFIXME - this probably won't work correctly.
					G_GetBoltPosition(NPCS.NPC, NPCS.NPC->NPC->genericBolt1, boltOrg, 0);
					AngleVectors( NPCS.NPC->client->ps.viewangles, fwd, NULL, NULL );
					G_PlayEffectID( G_EffectIndex( "howler/sonic" ), boltOrg, fwd);
					//G_PlayEffect( G_EffectIndex( "howler/sonic" ), NPCS.NPC->ghoul2, NPCS.NPC->NPC->genericBolt1, NPCS.NPC->s.number, NPCS.NPC->currentOrigin, 4750, qtrue );
					G_SoundOnEnt( NPCS.NPC, CHAN_VOICE, "sound/chars/howler/howl.mp3" );
					NPCS.NPC->count = 1;
				}
			}
		}
		break;
	default:
		//anims seem to get reset after a load, so just stop attacking and it will restart as needed.
		TIMER_Remove( NPCS.NPC, "attacking" );
		break;
	}

	// Just using this to remove the attacking flag at the right time
	TIMER_Done2( NPCS.NPC, "attacking", qtrue );
}

//----------------------------------
//replaced with SP version.
static void Howler_Combat( void )
{
	qboolean faced = qfalse;
	float distance;
	qboolean advance = qfalse;
	if ( NPCS.NPC->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//not on the ground
		if ( NPCS.NPC->client->ps.legsAnim == BOTH_JUMP1
			|| NPCS.NPC->client->ps.legsAnim == BOTH_INAIR1 )
		{//flying through the air with the greatest of ease, etc
			Howler_TryDamage( 10, qfalse, qfalse );
		}
	}
	else
	{//not in air, see if we should attack or advance
		// If we cannot see our target or we have somewhere to go, then do that
		if ( !NPC_ClearLOS4( NPCS.NPC->enemy ) )//|| UpdateGoal( ))
		{
			NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
			NPCS.NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range

			if ( NPCS.NPCInfo->localState == LSTATE_BERZERK )
			{
				NPC_Howler_Move( 3 );
			}
			else
			{
				NPC_Howler_Move( 10 );
			}
			NPC_UpdateAngles( qfalse, qtrue );
			return;
		}

		distance = DistanceHorizontal( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin );	

		if ( NPCS.NPC->enemy && NPCS.NPC->enemy->client && PM_InKnockDown( &NPCS.NPC->enemy->client->ps ) )
		{//get really close to knocked down enemies
			advance = (qboolean)( distance > MIN_DISTANCE ? qtrue : qfalse  );
		}
		else
		{
			advance = (qboolean)( distance > MAX_DISTANCE ? qtrue : qfalse  );//MIN_DISTANCE
		}

		if (( advance || NPCS.NPCInfo->localState == LSTATE_WAITING ) && TIMER_Done( NPCS.NPC, "attacking" )) // waiting monsters can't attack
		{
			if ( TIMER_Done2( NPCS.NPC, "takingPain", qtrue ))
			{
				NPCS.NPCInfo->localState = LSTATE_CLEAR;
			}
			else if ( TIMER_Done( NPCS.NPC, "standing" ) )
			{
				faced = Howler_Move( 1 );
			}
		}
		else
		{
			Howler_Attack( distance, qfalse );
		}
	}

	if ( !faced )
	{
		if ( //TIMER_Done( NPCS.NPC, "standing" ) //not just standing there
			//!advance //not moving
			TIMER_Done( NPCS.NPC, "attacking" ) )// not attacking
		{//not standing around
			// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
			NPC_FaceEnemy( qtrue );
		}
		else
		{
			NPC_UpdateAngles( qfalse, qtrue );
		}
	}
}

/*
-------------------------
NPC_Howler_Pain
-------------------------
*/
//replaced with SP code
void NPC_Howler_Pain( gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc ) 
{
	if ( !self || !self->NPC )
	{
		return;
	}

	if ( self->NPC->localState != LSTATE_BERZERK )//damage >= 10 )
	{
		self->NPC->stats.aggression += damage;
		self->NPC->localState = LSTATE_WAITING;

		TIMER_Remove( self, "attacking" );

		VectorCopy( self->NPC->lastPathAngles, self->s.angles );

		//if ( self->client->ps.legsAnim == BOTH_GESTURE1 )
		//RAFIXME - effects can't be stopped at this point
		/*
		{
			G_StopEffect( G_EffectIndex( "howler/sonic" ), self->playerModel, self->genericBolt1, self->s.number );
		}
		*/

		NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
		TIMER_Set( self, "takingPain", self->client->ps.legsTimer );//2900 );

		if ( self->health > HOWLER_PANIC_HEALTH )
		{//still have some health left
			if ( Q_irand( 0, self->NPC->stats.health ) > self->health )//FIXME: or check damage?
			{//back off!
				TIMER_Set( self, "standing", -level.time );
				TIMER_Set( self, "running", -level.time );
				TIMER_Set( self, "walking", -level.time );
				TIMER_Set( self, "retreating", Q_irand( 1000, 5000 ) );
			}
			else
			{//go after him!
				TIMER_Set( self, "standing", -level.time );
				TIMER_Set( self, "running", self->client->ps.legsTimer+Q_irand(3000,6000) );
				TIMER_Set( self, "walking", -level.time );
				TIMER_Set( self, "retreating", -level.time );
			}
		}
		else if ( self->NPC )
		{//panic!
			if ( Q_irand( 0, 1 ) )
			{//berzerk
				self->NPC->localState = LSTATE_BERZERK;
			}
			else
			{//flee
				self->NPC->localState = LSTATE_FLEE;
				TIMER_Set( self, "flee", Q_irand( 10000, 30000 ) );
			}
		}
	}
}


/*
-------------------------
NPC_BSHowler_Default
-------------------------
*/
//replaced with SP version.
void NPC_BSHowler_Default( void )
{
	if ( NPCS.NPC->client->ps.legsAnim != BOTH_GESTURE1 )
	{
		NPCS.NPC->count = 0;
	}
	//FIXME: if in jump, do damage in front and maybe knock them down?
	if ( !TIMER_Done( NPCS.NPC, "attacking" ) )
	{
		if ( NPCS.NPC->enemy )
		{
			//NPC_FaceEnemy( qfalse );
			Howler_Attack( Distance( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin ), qfalse );
		}
		else
		{
			//NPC_UpdateAngles( qfalse, qtrue );
			Howler_Attack( 0.0f, qfalse );
		}
		NPC_UpdateAngles( qfalse, qtrue );
		return;
	}

	if ( NPCS.NPC->enemy )
	{
		if ( NPCS.NPCInfo->stats.aggression > 0 )
		{
			if ( TIMER_Done( NPCS.NPC, "aggressionDecay" ) )
			{
				NPCS.NPCInfo->stats.aggression--;
				TIMER_Set( NPCS.NPC, "aggressionDecay", 500 );
			}
		}
		//RAFIXME - No fleeing, need to fix this at some point.
		/*
		if ( !TIMER_Done( NPCS.NPC, "flee" ) 
			&& NPC_BSFlee() )	//this can clear ENEMY
		{//successfully trying to run away
			return;
		}
		*/
		if ( NPCS.NPC->enemy == NULL)
		{
			NPC_UpdateAngles( qfalse, qtrue );
			return;
		}
		if ( NPCS.NPCInfo->localState == LSTATE_FLEE )
		{//we were fleeing, now done (either timer ran out or we cannot flee anymore
			if ( NPC_ClearLOS4( NPCS.NPC->enemy ) )
			{//if enemy is still around, go berzerk
				NPCS.NPCInfo->localState = LSTATE_BERZERK;
			}
			else
			{//otherwise, lick our wounds?
				NPCS.NPCInfo->localState = LSTATE_CLEAR;
				TIMER_Set( NPCS.NPC, "standing", Q_irand( 3000, 10000 ) );
			}
		}
		else if ( NPCS.NPCInfo->localState == LSTATE_BERZERK )
		{//go nuts!
		}
		else if ( NPCS.NPCInfo->stats.aggression >= Q_irand( 75, 125 ) )
		{//that's it, go nuts!
			NPCS.NPCInfo->localState = LSTATE_BERZERK;
		}
		else if ( !TIMER_Done( NPCS.NPC, "retreating" ) )
		{//trying to back off
			NPC_FaceEnemy( qtrue );
			if ( NPCS.NPC->client->ps.speed > NPCS.NPCInfo->stats.walkSpeed )
			{
				NPCS.NPC->client->ps.speed = NPCS.NPCInfo->stats.walkSpeed;
			}
			NPCS.ucmd.buttons |= BUTTON_WALKING;
			if ( Distance( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin ) < HOWLER_RETREAT_DIST )
			{//enemy is close
				vec3_t moveDir;
				AngleVectors( NPCS.NPC->r.currentAngles, moveDir, NULL, NULL );
				VectorScale( moveDir, -1, moveDir );
				if ( !NAV_DirSafe( NPCS.NPC, moveDir, 8 ) )
				{//enemy is backing me up against a wall or ledge!  Start to get really mad!
					NPCS.NPCInfo->stats.aggression += 2;
				}
				else
				{//back off
					NPCS.ucmd.forwardmove = -127;
				}
				//enemy won't leave me alone, get mad...
				NPCS.NPCInfo->stats.aggression++;
			}
			return;
		}
		else if ( TIMER_Done( NPCS.NPC, "standing" ) )
		{//not standing around
			if ( !(NPCS.NPCInfo->last_ucmd.forwardmove)
				&& !(NPCS.NPCInfo->last_ucmd.rightmove) )
			{//stood last frame
				if ( TIMER_Done( NPCS.NPC, "walking" ) 
					&& TIMER_Done( NPCS.NPC, "running" ) )
				{//not walking or running
					if ( Q_irand( 0, 2 ) )
					{//run for a while
						TIMER_Set( NPCS.NPC, "walking", Q_irand( 4000, 8000 ) );
					}
					else
					{//walk for a bit
						TIMER_Set( NPCS.NPC, "running", Q_irand( 2500, 5000 ) );
					}
				}
			}
			else if ( (NPCS.NPCInfo->last_ucmd.buttons&BUTTON_WALKING) )
			{//walked last frame
				if ( TIMER_Done( NPCS.NPC, "walking" ) )
				{//just finished walking
					if ( Q_irand( 0, 5 ) || DistanceSquared( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin ) < MAX_DISTANCE_SQR )
					{//run for a while
						TIMER_Set( NPCS.NPC, "running", Q_irand( 4000, 20000 ) );
					}
					else
					{//stand for a bit
						TIMER_Set( NPCS.NPC, "standing", Q_irand( 2000, 6000 ) );
					}
				}
			}
			else
			{//ran last frame
				if ( TIMER_Done( NPCS.NPC, "running" ) )
				{//just finished running
					if ( Q_irand( 0, 8 ) || DistanceSquared( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin ) < MAX_DISTANCE_SQR )
					{//walk for a while
						TIMER_Set( NPCS.NPC, "walking", Q_irand( 3000, 10000 ) );
					}
					else
					{//stand for a bit
						TIMER_Set( NPCS.NPC, "standing", Q_irand( 2000, 6000 ) );
					}
				}
			}
		}
		if ( NPC_ValidEnemy( NPCS.NPC->enemy ) == qfalse )
		{
			TIMER_Remove( NPCS.NPC, "lookForNewEnemy" );//make them look again right now
			if ( !NPCS.NPC->enemy->inuse || level.time - NPCS.NPC->enemy->s.time > Q_irand( 10000, 15000 ) )
			{//it's been a while since the enemy died, or enemy is completely gone, get bored with him
				NPCS.NPC->enemy = NULL;
				Howler_Patrol();
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
		}
		if ( TIMER_Done( NPCS.NPC, "lookForNewEnemy" ) )
		{
			gentity_t *sav_enemy = NPCS.NPC->enemy;//FIXME: what about NPCS.NPC->lastEnemy?
			gentity_t *newEnemy;
			NPCS.NPC->enemy = NULL;
			newEnemy = NPC_CheckEnemy( NPCS.NPCInfo->confusionTime < level.time, qfalse, qfalse );
			NPCS.NPC->enemy = sav_enemy;
			if ( newEnemy && newEnemy != sav_enemy )
			{//picked up a new enemy!
				NPCS.NPC->lastEnemy = NPCS.NPC->enemy;
				G_SetEnemy( NPCS.NPC, newEnemy );
				if ( NPCS.NPC->enemy != NPCS.NPC->lastEnemy )
				{//clear this so that we only sniff the player the first time we pick them up
					//RACC - this doesn't appear to get used for Howlers
					//NPCS.NPC->useDebounceTime = 0;
				}
				//hold this one for at least 5-15 seconds
				TIMER_Set( NPCS.NPC, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
			}
			else
			{//look again in 2-5 secs
				TIMER_Set( NPCS.NPC, "lookForNewEnemy", Q_irand( 2000, 5000 ) );
			}
		}
		Howler_Combat();
		if ( TIMER_Done( NPCS.NPC, "speaking" ) )
		{
			if ( !TIMER_Done( NPCS.NPC, "standing" )
				|| !TIMER_Done( NPCS.NPC, "retreating" ))
			{
				G_SoundOnEnt( NPCS.NPC, CHAN_VOICE, va( "sound/chars/howler/idle_hiss%d.mp3", Q_irand( 1, 2 ) ) );
			}
			else if ( !TIMER_Done( NPCS.NPC, "walking" ) 
				|| NPCS.NPCInfo->localState == LSTATE_FLEE )
			{
				G_SoundOnEnt( NPCS.NPC, CHAN_VOICE, va( "sound/chars/howler/howl_talk%d.mp3", Q_irand( 1, 5 ) ) );
			}
			else
			{
				G_SoundOnEnt( NPCS.NPC, CHAN_VOICE, va( "sound/chars/howler/howl_yell%d.mp3", Q_irand( 1, 5 ) ) );
			}
			if ( NPCS.NPCInfo->localState == LSTATE_BERZERK
				|| NPCS.NPCInfo->localState == LSTATE_FLEE )
			{
				TIMER_Set( NPCS.NPC, "speaking", Q_irand( 1000, 4000 ) );
			}
			else
			{
				TIMER_Set( NPCS.NPC, "speaking", Q_irand( 3000, 8000 ) );
			}
		}
		return;
	}
	else
	{
		if ( TIMER_Done( NPCS.NPC, "speaking" ) )
		{
			if ( !Q_irand( 0, 3 ) )
			{
				G_SoundOnEnt( NPCS.NPC, CHAN_VOICE, va( "sound/chars/howler/idle_hiss%d.mp3", Q_irand( 1, 2 ) ) );
			}
			else
			{
				G_SoundOnEnt( NPCS.NPC, CHAN_VOICE, va( "sound/chars/howler/howl_talk%d.mp3", Q_irand( 1, 5 ) ) );
			}
			TIMER_Set( NPCS.NPC, "speaking", Q_irand( 4000, 12000 ) );
		}
		if ( NPCS.NPCInfo->stats.aggression > 0 )
		{
			if ( TIMER_Done( NPCS.NPC, "aggressionDecay" ) )
			{
				NPCS.NPCInfo->stats.aggression--;
				TIMER_Set( NPCS.NPC, "aggressionDecay", 200 );
			}
		}
		if ( TIMER_Done( NPCS.NPC, "standing" ) )
		{//not standing around
			if ( !(NPCS.NPCInfo->last_ucmd.forwardmove)
				&& !(NPCS.NPCInfo->last_ucmd.rightmove) )
			{//stood last frame
				if ( TIMER_Done( NPCS.NPC, "walking" ) 
					&& TIMER_Done( NPCS.NPC, "running" ) )
				{//not walking or running
					if ( NPCS.NPCInfo->goalEntity )
					{//have somewhere to go
						if ( Q_irand( 0, 2 ) )
						{//walk for a while
							TIMER_Set( NPCS.NPC, "walking", Q_irand( 3000, 10000 ) );
						}
						else
						{//run for a bit
							TIMER_Set( NPCS.NPC, "running", Q_irand( 2500, 5000 ) );
						}
					}
				}
			}
			else if ( (NPCS.NPCInfo->last_ucmd.buttons&BUTTON_WALKING) )
			{//walked last frame
				if ( TIMER_Done( NPCS.NPC, "walking" ) )
				{//just finished walking
					if ( Q_irand( 0, 3 ) )
					{//run for a while
						TIMER_Set( NPCS.NPC, "running", Q_irand( 3000, 6000 ) );
					}
					else
					{//stand for a bit
						TIMER_Set( NPCS.NPC, "standing", Q_irand( 2500, 5000 ) );
					}
				}
			}
			else
			{//ran last frame
				if ( TIMER_Done( NPCS.NPC, "running" ) )
				{//just finished running
					if ( Q_irand( 0, 2 ) )
					{//walk for a while
						TIMER_Set( NPCS.NPC, "walking", Q_irand( 6000, 15000 ) );
					}
					else
					{//stand for a bit
						TIMER_Set( NPCS.NPC, "standing", Q_irand( 4000, 6000 ) );
					}
				}
			}
		}
		if ( NPCS.NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
		{
			Howler_Patrol();
		}
		else
		{
			Howler_Idle();
		}
	}

	NPC_UpdateAngles( qfalse, qtrue );
}
