//[CoOp]
#include "b_local.h"
//#include "Q3_Interface.h"

extern qboolean NPC_CheckSurrender( void );
extern void NPC_BehaviorSet_Default( int bState );

void NPC_BSCivilian_Default( int bState )
{
	if ( NPCS.NPC->enemy
		&& NPCS.NPC->s.weapon == WP_NONE 
		&& NPC_CheckSurrender() )
	{//surrendering, do nothing
	}
	else if ( NPCS.NPC->enemy 
		&& NPCS.NPC->s.weapon == WP_NONE 
		&& bState != BS_HUNT_AND_KILL 
		&& !trap->ICARUS_TaskIDPending( (sharedEntity_t *)NPCS.NPC, TID_MOVE_NAV ) )
	{//if in battle and have no weapon, run away, fixme: when in BS_HUNT_AND_KILL, they just stand there
		if ( !NPCS.NPCInfo->goalEntity
			|| bState != BS_FLEE //not fleeing
			//[CoOp]
			//RAFIXME - Impliment Nav code in that function
			|| ( NPCS.NPC->enemy//still have enemy (NPC_BSFlee checks enemy and can clear it)
			//|| ( NPC_BSFlee()//have reached our flee goal
			//	&& NPC->enemy//still have enemy (NPC_BSFlee checks enemy and can clear it)
			//[/CoOp]			
				&& DistanceSquared( NPCS.NPC->r.currentOrigin, NPCS.NPC->enemy->r.currentOrigin ) < 16384 )//enemy within 128
			)
		{//run away!
			NPC_StartFlee( NPCS.NPC->enemy, NPCS.NPC->enemy->r.currentOrigin, AEL_DANGER_GREAT, 5000, 10000 );
		}
	}
	else
	{//not surrendering
		//FIXME: if unarmed and a jawa/ugnuaght, constantly look for enemies/players to run away from?
		//FIXME: if we have a weapon and an enemy, set out playerTeam to the opposite of our enemy..???
		NPC_BehaviorSet_Default(bState);
	}
	if ( !VectorCompare( NPCS.NPC->client->ps.moveDir, vec3_origin ) )
	{//moving
		if ( NPCS.NPC->client->ps.legsAnim == BOTH_COWER1 )
		{//stop cowering anim on legs
			NPCS.NPC->client->ps.legsTimer = 0;
		}
	}
}
//[/CoOp]
