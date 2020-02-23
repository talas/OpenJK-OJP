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

//Rocket Trooper AI code
#include "g_local.h"
#include "b_local.h"

#define VELOCITY_DECAY		0.7f

#define RT_FLYING_STRAFE_VEL	60
#define RT_FLYING_STRAFE_DIS	200
#define RT_FLYING_UPWARD_PUSH	150

#define RT_FLYING_FORWARD_BASE_SPEED	50
#define RT_FLYING_FORWARD_MULTIPLIER	10

extern void NPC_BehaviorSet_Stormtrooper( int bState );
void RT_RunStormtrooperAI( void )
{
	int bState;
	//Execute our bState
	if(NPCS.NPCInfo->tempBehavior)
	{//Overrides normal behavior until cleared
		bState = NPCS.NPCInfo->tempBehavior;
	}
	else
	{
		if(!NPCS.NPCInfo->behaviorState)
			NPCS.NPCInfo->behaviorState = NPCS.NPCInfo->defaultBehavior;

		bState = NPCS.NPCInfo->behaviorState;
	}
	NPC_BehaviorSet_Stormtrooper( bState );
}

extern void Boba_FlyStart( gentity_t *self );
void RT_FireDecide( void )
{
	qboolean enemyLOS = qfalse;
	qboolean enemyCS = qfalse;
	qboolean enemyInFOV = qfalse;
	//qboolean move = qtrue;
	qboolean shoot = qfalse;
	qboolean hitAlly = qfalse;
	vec3_t	impactPos;
	float	enemyDist;

	if ( NPCS.NPC->client->ps.groundEntityNum == ENTITYNUM_NONE
		&& NPCS.NPC->client->ps.fd.forceJumpZStart
		&& !BG_FlippingAnim( NPCS.NPC->client->ps.legsAnim )
		&& !Q_irand( 0, 10 ) )
	{//take off
		Boba_FlyStart( NPCS.NPC );
	}

	if ( !NPCS.NPC->enemy )
	{
		return;
	}

	VectorClear( impactPos );
	enemyDist = DistanceSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin );

	vec3_t	enemyDir, shootDir;
	VectorSubtract( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, enemyDir );
	VectorNormalize( enemyDir );
	AngleVectors( NPCS.NPC->client->ps.viewangles, shootDir, NULL, NULL );
	float dot = DotProduct( enemyDir, shootDir );
	if ( dot > 0.5f ||( enemyDist * (1.0f-dot)) < 10000 )
	{//enemy is in front of me or they're very close and not behind me
		enemyInFOV = qtrue;
	}

	if ( enemyDist < MIN_ROCKET_DIST_SQUARED )//128
	{//enemy within 128
		if ( (NPCS.NPC->client->ps.weapon == WP_FLECHETTE || NPCS.NPC->client->ps.weapon == WP_REPEATER) &&
			(NPCS.NPCInfo->scriptFlags & SCF_ALT_FIRE) )
		{//shooting an explosive, but enemy too close, switch to primary fire
			NPCS.NPCInfo->scriptFlags &= ~SCF_ALT_FIRE;
			//FIXME: we can never go back to alt-fire this way since, after this, we don't know if we were initially supposed to use alt-fire or not...
		}
	}

	//can we see our target?
	if ( TIMER_Done( NPCS.NPC, "nextAttackDelay" ) && TIMER_Done( NPCS.NPC, "flameTime" ) )
	{
		if ( NPC_ClearLOS4( NPCS.NPC->enemy ) )
		{
			NPCS.NPCInfo->enemyLastSeenTime = level.time;
			enemyLOS = qtrue;

			if ( NPCS.NPC->client->ps.weapon == WP_NONE )
			{
				enemyCS = qfalse;//not true, but should stop us from firing
			}
			else
			{//can we shoot our target?
				if ( (NPCS.NPC->client->ps.weapon == WP_ROCKET_LAUNCHER
					|| (NPCS.NPC->client->ps.weapon == WP_CONCUSSION && !(NPCS.NPCInfo->scriptFlags&SCF_ALT_FIRE))
					|| (NPCS.NPC->client->ps.weapon == WP_FLECHETTE && (NPCS.NPCInfo->scriptFlags&SCF_ALT_FIRE))) && enemyDist < MIN_ROCKET_DIST_SQUARED )//128*128
				{
					enemyCS = qfalse;//not true, but should stop us from firing
					hitAlly = qtrue;//us!
					//FIXME: if too close, run away!
				}
				else if ( enemyInFOV )
				{//if enemy is FOV, go ahead and check for shooting
					int hit = NPC_ShotEntity( NPCS.NPC->enemy, impactPos );
					gentity_t *hitEnt = &g_entities[hit];

					if ( hit == NPCS.NPC->enemy->s.number
						|| ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPCS.NPC->client->enemyTeam )
						|| ( hitEnt && hitEnt->takedamage && ((hitEnt->r.svFlags&SVF_GLASS_BRUSH)||hitEnt->health < 40||NPCS.NPC->s.weapon == WP_EMPLACED_GUN) ) )
					{//can hit enemy or enemy ally or will hit glass or other minor breakable (or in emplaced gun), so shoot anyway
						enemyCS = qtrue;
						//NPC_AimAdjust( 2 );//adjust aim better longer we have clear shot at enemy
						VectorCopy( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPCInfo->enemyLastSeenLocation );
					}
					else
					{//Hmm, have to get around this bastard
						//NPC_AimAdjust( 1 );//adjust aim better longer we can see enemy
						if ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPCS.NPC->client->playerTeam )
						{//would hit an ally, don't fire!!!
							hitAlly = qtrue;
						}
						else
						{//Check and see where our shot *would* hit... if it's not close to the enemy (within 256?), then don't fire
						}
					}
				}
				else
				{
					enemyCS = qfalse;//not true, but should stop us from firing
				}
			}
		}
		else if ( trap->InPVS( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin ) )
		{
			NPCS.NPCInfo->enemyLastSeenTime = level.time;
			//NPC_AimAdjust( -1 );//adjust aim worse longer we cannot see enemy
		}

		if ( NPCS.NPC->client->ps.weapon == WP_NONE )
		{
			shoot = qfalse;
		}
		else
		{
			if ( enemyCS )
			{
				shoot = qtrue;
			}
		}

		if ( !enemyCS )
		{//if have a clear shot, always try
			//See if we should continue to fire on their last position
			//!TIMER_Done( NPC, "stick" ) ||
			if ( !hitAlly //we're not going to hit an ally
				&& enemyInFOV //enemy is in our FOV //FIXME: or we don't have a clear LOS?
				&& NPCS.NPCInfo->enemyLastSeenTime > 0 )//we've seen the enemy
			{
				if ( level.time - NPCS.NPCInfo->enemyLastSeenTime < 10000 )//we have seem the enemy in the last 10 seconds
				{
					if ( !Q_irand( 0, 10 ) )
					{
						//Fire on the last known position
						vec3_t	muzzle, dir, angles;
						qboolean tooClose = qfalse;
						qboolean tooFar = qfalse;

						CalcEntitySpot( NPCS.NPC, SPOT_HEAD, muzzle );
						if ( VectorCompare( impactPos, vec3_origin ) )
						{//never checked ShotEntity this frame, so must do a trace...
							trace_t tr;
							//vec3_t	mins = {-2,-2,-2}, maxs = {2,2,2};
							vec3_t	forward, end;
							AngleVectors( NPCS.NPC->client->ps.viewangles, forward, NULL, NULL );
							VectorMA( muzzle, 8192, forward, end );
							trap->Trace( &tr, muzzle, vec3_origin, vec3_origin, end, NPCS.NPC->s.number, MASK_SHOT, qfalse, 0, 0 );
							VectorCopy( tr.endpos, impactPos );
						}

						//see if impact would be too close to me
						float distThreshold = 16384/*128*128*/;//default
						switch ( NPCS.NPC->s.weapon )
						{
						case WP_ROCKET_LAUNCHER:
						case WP_FLECHETTE:
						case WP_THERMAL:
						case WP_TRIP_MINE:
						case WP_DET_PACK:
							distThreshold = 65536/*256*256*/;
							break;
						case WP_REPEATER:
							if ( NPCS.NPCInfo->scriptFlags&SCF_ALT_FIRE )
							{
								distThreshold = 65536/*256*256*/;
							}
							break;
						case WP_CONCUSSION:
							if ( !(NPCS.NPCInfo->scriptFlags&SCF_ALT_FIRE) )
							{
								distThreshold = 65536/*256*256*/;
							}
							break;
						default:
							break;
						}

						float dist = DistanceSquared( impactPos, muzzle );

						if ( dist < distThreshold )
						{//impact would be too close to me
							tooClose = qtrue;
						}
						else if ( level.time - NPCS.NPCInfo->enemyLastSeenTime > 5000 ||
							(NPCS.NPCInfo->group && level.time - NPCS.NPCInfo->group->lastSeenEnemyTime > 5000 ))
						{//we've haven't seen them in the last 5 seconds
							//see if it's too far from where he is
							distThreshold = 65536/*256*256*/;//default
							switch ( NPCS.NPC->s.weapon )
							{
							case WP_ROCKET_LAUNCHER:
							case WP_FLECHETTE:
							case WP_THERMAL:
							case WP_TRIP_MINE:
							case WP_DET_PACK:
								distThreshold = 262144/*512*512*/;
								break;
							case WP_REPEATER:
								if ( NPCS.NPCInfo->scriptFlags&SCF_ALT_FIRE )
								{
									distThreshold = 262144/*512*512*/;
								}
								break;
							case WP_CONCUSSION:
								if ( !(NPCS.NPCInfo->scriptFlags&SCF_ALT_FIRE) )
								{
									distThreshold = 262144/*512*512*/;
								}
								break;
							default:
								break;
							}
							dist = DistanceSquared( impactPos, NPCS.NPCInfo->enemyLastSeenLocation );
							if ( dist > distThreshold )
							{//impact would be too far from enemy
								tooFar = qtrue;
							}
						}

						if ( !tooClose && !tooFar )
						{//okay too shoot at last pos
							VectorSubtract( NPCS.NPCInfo->enemyLastSeenLocation, muzzle, dir );
							VectorNormalize( dir );
							vectoangles( dir, angles );

							NPCS.NPCInfo->desiredYaw		= angles[YAW];
							NPCS.NPCInfo->desiredPitch	= angles[PITCH];

							shoot = qtrue;
						}
					}
				}
			}
		}

		//FIXME: don't shoot right away!
		// 74145: implement fireDelay?
		//if ( NPCS.NPC->client->fireDelay )
		if ( NPCS.NPC->client->ps.weaponTime > 0 )
		{
			if ( NPCS.NPC->s.weapon == WP_ROCKET_LAUNCHER
				|| (NPCS.NPC->s.weapon == WP_CONCUSSION&&!(NPCS.NPCInfo->scriptFlags&SCF_ALT_FIRE)) )
			{
				if ( !enemyLOS || !enemyCS )
				{//cancel it
					NPCS.NPC->client->ps.weaponTime = 0;
					//NPCS.NPC->client->fireDelay = 0;
				}
				else
				{//delay our next attempt
					TIMER_Set( NPCS.NPC, "nextAttackDelay", Q_irand( 1000, 3000 ) );//FIXME: base on g_spskill
				}
			}
		}
		else if ( shoot )
		{//try to shoot if it's time
			if ( TIMER_Done( NPCS.NPC, "nextAttackDelay" ) )
			{
				if( !(NPCS.NPCInfo->scriptFlags & SCF_FIRE_WEAPON) ) // we've already fired, no need to do it again here
				{
					WeaponThink( qtrue );
				}
				//NASTY
				int altChance = 6;//FIXME: base on g_spskill
				if ( NPCS.NPC->s.weapon == WP_ROCKET_LAUNCHER )
				{
					if ( (NPCS.ucmd.buttons&BUTTON_ATTACK)
						&& !Q_irand( 0, altChance ) )
					{//every now and then, shoot a homing rocket
						NPCS.ucmd.buttons &= ~BUTTON_ATTACK;
						NPCS.ucmd.buttons |= BUTTON_ALT_ATTACK;
						//NPCS.NPC->client->fireDelay = Q_irand( 1000, 3000 );//FIXME: base on g_spskill
						NPCS.NPC->client->ps.weaponTime = Q_irand( 1000, 3000 );
					}
				}
				else if ( NPCS.NPC->s.weapon == WP_CONCUSSION )
				{
					if ( (NPCS.ucmd.buttons&BUTTON_ATTACK)
						&& Q_irand( 0, altChance*5 ) )
					{//fire the beam shot
						NPCS.ucmd.buttons &= ~BUTTON_ATTACK;
						NPCS.ucmd.buttons |= BUTTON_ALT_ATTACK;
						TIMER_Set( NPCS.NPC, "nextAttackDelay", Q_irand( 1500, 2500 ) );//FIXME: base on g_spskill
					}
					else
					{//fire the rocket-like shot
						TIMER_Set( NPCS.NPC, "nextAttackDelay", Q_irand( 3000, 5000 ) );//FIXME: base on g_spskill
					}
				}
			}
		}
	}
}

//=====================================================================================
//FLYING behavior 
//=====================================================================================
qboolean RT_Flying( gentity_t *self )
{
	return ((qboolean)(self->client->ps.eFlags2 & EF2_FLYING));
}

void RT_Flying_ApplyFriction( float frictionScale )
{
	if ( NPCS.NPC->client->ps.velocity[0] )
	{
		NPCS.NPC->client->ps.velocity[0] *= VELOCITY_DECAY;///frictionScale;

		if ( fabs( NPCS.NPC->client->ps.velocity[0] ) < 1 )
		{
			NPCS.NPC->client->ps.velocity[0] = 0;
		}
	}

	if ( NPCS.NPC->client->ps.velocity[1] )
	{
		NPCS.NPC->client->ps.velocity[1] *= VELOCITY_DECAY;///frictionScale;

		if ( fabs( NPCS.NPC->client->ps.velocity[1] ) < 1 )
		{
			NPCS.NPC->client->ps.velocity[1] = 0;
		}
	}
}

void RT_Flying_MaintainHeight( void )
{
	float	dif = 0;

	// Update our angles regardless
	NPC_UpdateAngles( qtrue, qtrue );

	/*
	  // 74145: Implement?
	if ( NPCS.NPC->forcePushTime > level.time )
	{//if being pushed, we don't have control over our movement
		return;
	}
	*/

	if ( (NPCS.NPC->client->ps.pm_flags&PMF_TIME_KNOCKBACK) )
	{//don't slow down for a bit
		if ( NPCS.NPC->client->ps.pm_time > 0 )
		{
			VectorScale( NPCS.NPC->client->ps.velocity, 0.9f, NPCS.NPC->client->ps.velocity );
			return;
		}
	}

	/*
	if ( (NPC->client->ps.eFlags&EF_FORCE_GRIPPED) )
	{
		RT_Flying_ApplyFriction( 3.0f );
		return;
	}
	*/
	// If we have an enemy, we should try to hover at or a little below enemy eye level
	if ( NPCS.NPC->enemy
	     && (!trap->ICARUS_TaskIDPending( (sharedEntity_t*)NPCS.NPC, TID_MOVE_NAV ) || !NPCS.NPCInfo->goalEntity ) )
	{
		if (TIMER_Done( NPCS.NPC, "heightChange" ))
		{
			TIMER_Set( NPCS.NPC,"heightChange",Q_irand( 1000, 3000 ));

			float enemyZHeight = NPCS.NPC->enemy->r.currentOrigin[2];
			if ( NPCS.NPC->enemy->client
				&& NPCS.NPC->enemy->client->ps.groundEntityNum == ENTITYNUM_NONE
				&& (NPCS.NPC->enemy->client->ps.fd.forcePowersActive&(1<<FP_LEVITATION)) )
			{//so we don't go up when they force jump up at us
				enemyZHeight = NPCS.NPC->enemy->client->ps.fd.forceJumpZStart;
			}

			// Find the height difference
			dif = (enemyZHeight +  Q_flrand( NPCS.NPC->enemy->r.maxs[2]/2, NPCS.NPC->enemy->r.maxs[2]+8 )) - NPCS.NPC->r.currentOrigin[2];

			float	difFactor = 10.0f;

			// cap to prevent dramatic height shifts
			if ( fabs( dif ) > 2*difFactor )
			{
				if ( fabs( dif ) > 20*difFactor )
				{
					dif = ( dif < 0 ? -20*difFactor : 20*difFactor );
				}

				NPCS.NPC->client->ps.velocity[2] = (NPCS.NPC->client->ps.velocity[2]+dif)/2;
			}
			NPCS.NPC->client->ps.velocity[2] *= Q_flrand( 0.85f, 1.25f );
		}
		else
		{//don't get too far away from height of enemy...
			float enemyZHeight = NPCS.NPC->enemy->r.currentOrigin[2];
			if ( NPCS.NPC->enemy->client
				&& NPCS.NPC->enemy->client->ps.groundEntityNum == ENTITYNUM_NONE
				&& (NPCS.NPC->enemy->client->ps.fd.forcePowersActive&(1<<FP_LEVITATION)) )
			{//so we don't go up when they force jump up at us
				enemyZHeight = NPCS.NPC->enemy->client->ps.fd.forceJumpZStart;
			}
			dif = NPCS.NPC->r.currentOrigin[2] - (enemyZHeight+64);
			float maxHeight = 200;
			float hDist = DistanceHorizontal( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin );
			if ( hDist < 512 )
			{
				maxHeight *= hDist/512;
			}
			if ( dif > maxHeight )
			{
				if ( NPCS.NPC->client->ps.velocity[2] > 0 )//FIXME: or: we can't see him anymore
				{//slow down
					if ( NPCS.NPC->client->ps.velocity[2] )
					{
						NPCS.NPC->client->ps.velocity[2] *= VELOCITY_DECAY;

						if ( fabs( NPCS.NPC->client->ps.velocity[2] ) < 2 )
						{
							NPCS.NPC->client->ps.velocity[2] = 0;
						}
					}
				}
				else
				{//start coming back down
					NPCS.NPC->client->ps.velocity[2] -= 4;
				}
			}
			else if ( dif < -200 && NPCS.NPC->client->ps.velocity[2] < 0 )//we're way below him
			{
				if ( NPCS.NPC->client->ps.velocity[2] < 0 )//FIXME: or: we can't see him anymore
				{//slow down
					if ( NPCS.NPC->client->ps.velocity[2] )
					{
						NPCS.NPC->client->ps.velocity[2] *= VELOCITY_DECAY;

						if ( fabs( NPCS.NPC->client->ps.velocity[2] ) > -2 )
						{
							NPCS.NPC->client->ps.velocity[2] = 0;
						}
					}
				}
				else
				{//start going back up
					NPCS.NPC->client->ps.velocity[2] += 4;
				}
			}
		}
	}
	else
	{
		gentity_t *goal = NULL;

		if ( NPCS.NPCInfo->goalEntity )	// Is there a goal?
		{
			goal = NPCS.NPCInfo->goalEntity;
		}
		else
		{
			goal = NPCS.NPCInfo->lastGoalEntity;
		}
		if ( goal )
		{
			dif = goal->r.currentOrigin[2] - NPCS.NPC->r.currentOrigin[2];
		}
		else if ( VectorCompare( NPCS.NPC->pos1, vec3_origin ) )
		{//have a starting position as a reference point
			dif = NPCS.NPC->pos1[2] - NPCS.NPC->r.currentOrigin[2];
		}

		if ( fabs( dif ) > 24 )
		{
			NPCS.ucmd.upmove = ( NPCS.ucmd.upmove < 0 ? -4 : 4 );
		}
		else
		{
			if ( NPCS.NPC->client->ps.velocity[2] )
			{
				NPCS.NPC->client->ps.velocity[2] *= VELOCITY_DECAY;

				if ( fabs( NPCS.NPC->client->ps.velocity[2] ) < 2 )
				{
					NPCS.NPC->client->ps.velocity[2] = 0;
				}
			}
		}
	}

	// Apply friction
	RT_Flying_ApplyFriction( 1.0f );
}

void RT_Flying_Strafe( void )
{
	int		side;
	vec3_t	end, right, dir;
	trace_t	tr;

	if ( Q_flrand(0.0f, 1.0f) > 0.7f
		|| !NPCS.NPC->enemy
		|| !NPCS.NPC->enemy->client )
	{
		// Do a regular style strafe
		AngleVectors( NPCS.NPC->client->renderInfo.eyeAngles, NULL, right, NULL );

		// Pick a random strafe direction, then check to see if doing a strafe would be
		//	reasonably valid
		side = ( rand() & 1 ) ? -1 : 1;
		VectorMA( NPCS.NPC->r.currentOrigin, RT_FLYING_STRAFE_DIS * side, right, end );

		trap->Trace( &tr, NPCS.NPC->r.currentOrigin, NULL, NULL, end, NPCS.NPC->s.number, MASK_SOLID, qfalse, 0, 0 );

		// Close enough
		if ( tr.fraction > 0.9f )
		{
			float vel = RT_FLYING_STRAFE_VEL+Q_flrand(-20,20);
			VectorMA( NPCS.NPC->client->ps.velocity, vel*side, right, NPCS.NPC->client->ps.velocity );
			if ( !Q_irand( 0, 3 ) )
			{
				// Add a slight upward push
				float upPush = RT_FLYING_UPWARD_PUSH;
				if ( NPCS.NPC->client->ps.velocity[2] < 300 )
				{
					if ( NPCS.NPC->client->ps.velocity[2] < 300+upPush )
					{
						NPCS.NPC->client->ps.velocity[2] += upPush;
					}
					else
					{
						NPCS.NPC->client->ps.velocity[2] = 300;
					}
				}
			}

			NPCS.NPCInfo->standTime = level.time + 1000 + Q_flrand(0.0f, 1.0f) * 500;
		}
	}
	else
	{
		// Do a strafe to try and keep on the side of their enemy
		AngleVectors( NPCS.NPC->enemy->client->renderInfo.eyeAngles, dir, right, NULL );

		// Pick a random side
		side = ( rand() & 1 ) ? -1 : 1;
		float	stDis = RT_FLYING_STRAFE_DIS*2.0f;
		VectorMA( NPCS.NPC->enemy->r.currentOrigin, stDis * side, right, end );

		// then add a very small bit of random in front of/behind the player action
		VectorMA( end, Q_flrand(-1.0f, 1.0f) * 25, dir, end );

		trap->Trace( &tr, NPCS.NPC->r.currentOrigin, NULL, NULL, end, NPCS.NPC->s.number, MASK_SOLID, qfalse, 0, 0 );

		// Close enough
		if ( tr.fraction > 0.9f )
		{
			float vel = (RT_FLYING_STRAFE_VEL*4)+Q_flrand(-20,20);
			VectorSubtract( tr.endpos, NPCS.NPC->r.currentOrigin, dir );
			dir[2] *= 0.25; // do less upward change
			float dis = VectorNormalize( dir );
			if ( dis > vel )
			{
				dis = vel;
			}
			// Try to move the desired enemy side
			VectorMA( NPCS.NPC->client->ps.velocity, dis, dir, NPCS.NPC->client->ps.velocity );

			if ( !Q_irand( 0, 3 ) )
			{
				float upPush = RT_FLYING_UPWARD_PUSH;
				// Add a slight upward push
				if ( NPCS.NPC->client->ps.velocity[2] < 300 )
				{
					if ( NPCS.NPC->client->ps.velocity[2] < 300+upPush )
					{
						NPCS.NPC->client->ps.velocity[2] += upPush;
					}
					else
					{
						NPCS.NPC->client->ps.velocity[2] = 300;
					}
				}
				else if ( NPCS.NPC->client->ps.velocity[2] > 300 )
				{
					NPCS.NPC->client->ps.velocity[2] = 300;
				}
			}

			NPCS.NPCInfo->standTime = level.time + 2500 + Q_flrand(0.0f, 1.0f) * 500;
		}
	}
}

void RT_Flying_Hunt( qboolean visible, qboolean advance )
{
	float	distance, speed;
	vec3_t	forward;

	/* 74145: Implement?
	if ( NPCS.NPC->forcePushTime >= level.time )
		//|| (NPC->client->ps.eFlags&EF_FORCE_GRIPPED) )
	{//if being pushed, we don't have control over our movement
		NPCS.NPC->delay = 0;
		return;
	}
	*/
	NPC_FaceEnemy( qtrue );

	// If we're not supposed to stand still, pursue the player
	if ( NPCS.NPCInfo->standTime < level.time )
	{
		// Only strafe when we can see the player
		if ( visible )
		{
			NPCS.NPC->delay = 0;
			RT_Flying_Strafe();
			return;
		}
	}

	// If we don't want to advance, stop here
	if ( advance  )
	{
		// Only try and navigate if the player is visible
		if ( visible == qfalse )
		{
			// Move towards our goal
			NPCS.NPCInfo->goalEntity = NPCS.NPC->enemy;
			NPCS.NPCInfo->goalRadius = 24;

			NPCS.NPC->delay = 0;
			NPC_MoveToGoal(qtrue);
			return;

		}
	}
	//else move straight at/away from him
	VectorSubtract( NPCS.NPC->enemy->r.currentOrigin, NPCS.NPC->r.currentOrigin, forward );
	forward[2] *= 0.1f;
	distance = VectorNormalize( forward );

	speed = RT_FLYING_FORWARD_BASE_SPEED + RT_FLYING_FORWARD_MULTIPLIER * g_npcspskill.integer;
	if ( advance && distance < Q_flrand( 256, 3096 ) )
	{
		NPCS.NPC->delay = 0;
		VectorMA( NPCS.NPC->client->ps.velocity, speed, forward, NPCS.NPC->client->ps.velocity );
	}
	else if ( distance < Q_flrand( 0, 128 ) )
	{
		if ( NPCS.NPC->health <= 50 )
		{//always back off
			NPCS.NPC->delay = 0;
		}
		else if ( !TIMER_Done( NPCS.NPC, "backoffTime" ) )
		{//still backing off from end of last delay
			NPCS.NPC->delay = 0;
		}
		else if ( !NPCS.NPC->delay )
		{//start a new delay
			NPCS.NPC->delay = Q_irand( 0, 10+(20*(2-g_npcspskill.integer)) );
		}
		else
		{//continue the current delay
			NPCS.NPC->delay--;
		}
		if ( !NPCS.NPC->delay )
		{//delay done, now back off for a few seconds!
			TIMER_Set( NPCS.NPC, "backoffTime", Q_irand( 2000, 5000 ) );
			VectorMA( NPCS.NPC->client->ps.velocity, speed*-2, forward, NPCS.NPC->client->ps.velocity );
		}
	}
	else
	{
		NPCS.NPC->delay = 0;
	}
}

void RT_Flying_Ranged( qboolean visible, qboolean advance )
{
	if ( NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
	{
		RT_Flying_Hunt( visible, advance );
	}
}

void RT_Flying_Attack( void )
{
	// Always keep a good height off the ground
	RT_Flying_MaintainHeight();

	// Rate our distance to the target, and our visibilty
	float		distance	= DistanceHorizontalSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin );
	qboolean	visible		= NPC_ClearLOS4( NPCS.NPC->enemy );
	qboolean	advance		= (qboolean)(distance>(256.0f*256.0f));

	// If we cannot see our target, move to see it
	if ( visible == qfalse )
	{
		if ( NPCS.NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
		{
			RT_Flying_Hunt( visible, advance );
			return;
		}
	}

	RT_Flying_Ranged( visible, advance );
}

extern gentity_t *NPC_CheckEnemy( qboolean findNew, qboolean tooFarOk, qboolean setEnemy );
void RT_Flying_Think( void )
{
	// 74145: Update selected enemy as needed
	if (!NPCS.NPC->enemy || NPCS.NPC->enemy->client)
		NPC_CheckEnemy(qtrue, qfalse, qtrue);

	if ( trap->ICARUS_TaskIDPending( (sharedEntity_t*)NPCS.NPC, TID_MOVE_NAV )
		&& UpdateGoal() )
	{//being scripted to go to a certain spot, don't maintain height
		if ( NPC_MoveToGoal( qtrue ) )
		{//we could macro-nav to our goal
			if ( NPCS.NPC->enemy && NPCS.NPC->enemy->health && NPCS.NPC->enemy->inuse )
			{
				NPC_FaceEnemy( qtrue );
				RT_FireDecide();
			}
		}
		else
		{//frick, no where to nav to, keep us in the air!
			RT_Flying_MaintainHeight();
		}
		return;
	}

	if ( NPCS.NPC->random == 0.0f )
	{
		// used to offset seekers around a circle so they don't occupy the same spot.  This is not a fool-proof method.
		NPCS.NPC->random = Q_flrand(0.0f, 1.0f) * 6.3f; // roughly 2pi
	}

	if ( NPCS.NPC->enemy && NPCS.NPC->enemy->health && NPCS.NPC->enemy->inuse )
	{
		RT_Flying_Attack();
		RT_FireDecide();
		return;
	}
	else
	{
		RT_Flying_MaintainHeight();
		RT_RunStormtrooperAI();
		return;
	}
}


//=====================================================================================
//ON GROUND WITH ENEMY behavior
//=====================================================================================

//=====================================================================================
//DEFAULT behavior
//=====================================================================================

extern void RT_CheckJump( void );
void NPC_BSRT_Default( void )
{
	//FIXME: custom pain and death funcs:
		//pain3 is in air
		//die in air is both_falldeath1
		//attack1 is on ground, attack2 is in air

	//FIXME: this doesn't belong here
	if ( NPCS.NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
	{
		if ( NPCS.NPCInfo->rank >= RANK_LT )//&& !Q_irand( 0, 50 ) )
		{//officers always stay in the air
			NPCS.NPC->client->ps.velocity[2] = Q_irand( 50, 125 );
			NPCS.NPC->NPC->aiFlags |= NPCAI_FLY;	//fixme also, Inform NPC_HandleAIFlags we want to fly
		}
	}

	if ( RT_Flying( NPCS.NPC ) )
	{//FIXME: only officers need do this, right?
		RT_Flying_Think();
	}
	else if ( NPCS.NPC->enemy != NULL )
	{//rocketrooper on ground with enemy
		UpdateGoal();
		RT_RunStormtrooperAI();
		RT_CheckJump();
		//NPC_BSST_Default();//FIXME: add missile avoidance
		//RT_Hunt();//NPC_BehaviorSet_Jedi( bState );
	}
	else
	{//shouldn't have gotten in here
		RT_RunStormtrooperAI();
		//NPC_BSST_Patrol();
	}
}
