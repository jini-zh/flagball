#include <string.h>
#include <engine/shared/config.h>
#include <engine/server.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/gamecontext.h>
#include <game/collision.h>
#include <generated/server_data.h>
#include "fb.h"

CBall::CBall(CGameWorld *pGameWorld, int Team, vec2 Pos)
: CFlag(pGameWorld, Team, Pos)
{
	m_Pos = Pos;
}

void CBall::Reset()
{
	CFlag::Reset();
	m_pLastCarrier = NULL;
	m_IdleTick = -1;
	m_LastCarrierTeam = GetTeam();
}

void CBall::TickDefered()
{
	CCharacter *carrier = GetCarrier();
	if(carrier)
	{
		// update flag position
		m_Pos = carrier->GetPos();
	} 
	else if(!IsAtStand())
	{
		// flag is out of the map or is lying too long
		if (GameLayerClipped(m_Pos) || (m_IdleTick >= 0 && Server()->Tick() > m_IdleTick + Server()->TickSpeed() * g_Config.m_SvfbBallResetTime))
		{
			Reset();
			GameServer()->m_pController->OnFlagReturn(this);
		}
		else
		{
			vec2& vel = Vel();
			// do ball physics
			vel.y += GameServer()->m_World.m_Core.m_Tuning.m_Gravity;
			if (vel.x > -0.1f && vel.x < 0.1f && g_Config.m_SvfbBallFriction != 0)
				vel.x = 0.0f;
			else if (vel.x < 0)
				vel.x += g_Config.m_SvfbBallFriction / 10000.f;
			else
				vel.x -= g_Config.m_SvfbBallFriction / 10000.f;

			float vy = vel.y;
			GameServer()->Collision()->MoveBox(&m_Pos, &vel, vec2(ms_PhysSize, ms_PhysSize), 0.5f);
			if (vel.x == 0 && (vy > 0) != (vel.y > 0) && abs(vel.y) < GameServer()->m_World.m_Core.m_Tuning.m_Gravity)
			{
				if (m_IdleTick == -1)
					m_IdleTick = Server()->Tick();
			}
			else
				m_IdleTick = -1;
		}
	}
}


void CBall::Hit(CPlayer *Player, vec2 Direction)
{
	Kick(Direction * g_Config.m_SvfbLaserMomentum * 0.1f);
	m_pLastCarrier = Player;
	m_LastCarrierTeam = Player->GetTeam();

	CBall* b = this;
	static_cast<CGameControllerFB *>(GameServer()->m_pController)->SetBallColor(b, m_LastCarrierTeam);
}

void CBall::Release(vec2 Velocity)
{
	m_IdleTick = -1;
	m_pLastCarrier = GetCarrier()->GetPlayer();
	m_LastCarrierTeam = m_pLastCarrier->GetTeam();
	Drop();
	Vel() = Velocity;
}

CGameControllerFB::CGameControllerFB(class CGameContext *pGameServer) 
: IGameController(pGameServer)
{
	m_apBalls[0] = NULL;
	m_apBalls[1] = NULL;
	m_Instagib = g_Config.m_SvfbInstagib;
	m_pGameType = m_Instagib ? "iFB" : "FB";
	m_GameFlags = GAMEFLAG_TEAMS | GAMEFLAG_FLAGS;
	m_apGoalPos[0] = vec2(0, 0);
	m_apGoalPos[1] = vec2(0, 0);
	m_NumGoals = 0;
	g_pData->m_Weapons.m_aId[WEAPON_LASER].m_Damage = g_Config.m_SvfbInstagib ? 10 : 5;
	
}

bool CGameControllerFB::OnEntity (int index, vec2 pos)
{
	//	don't add pickups in instagib
	if (m_Instagib && index > ENTITY_FLAGSTAND_BLUE)
		return false;
	
	if (IGameController::OnEntity (index, pos))
		return true;
	
	//	add goal positions and balls
	switch (index)
	{
		case ENTITY_SPAWN:
			if (m_NumGoals < FB_MAX_GOALS)
				m_apGoalPos[m_NumGoals++] = pos;
			break;
		case ENTITY_FLAGSTAND_RED:
			if (!m_apBalls[TEAM_RED])
				m_apBalls[TEAM_RED] = new CBall(&GameServer()->m_World, TEAM_RED, pos);
			break;
		case ENTITY_FLAGSTAND_BLUE:
			if (!m_apBalls[TEAM_BLUE])
				m_apBalls[TEAM_BLUE] = new CBall(&GameServer()->m_World, TEAM_BLUE, pos);
			break;
		default:
			return false;
	}
	return true;
	
}

int CGameControllerFB::OnCharacterDeath(class CCharacter *victim, class CPlayer *killer, int weaponid)
{
	
	// do scoreing
	if (weaponid == WEAPON_SELF || weaponid == WEAPON_WORLD)
		victim->GetPlayer()->m_Score--;	// suicide or death-tile
	else if (killer && weaponid != WEAPON_GAME)
		{
			if (IsTeamplay() && victim->GetPlayer()->GetTeam() == killer->GetTeam())
				killer->m_Score--;	// teamkill
			else
				killer->m_Score++;	// normal kill
		}

	int mode = 0;
	if (killer)
	{
		CCharacter *const ch = killer->GetCharacter();
		if (ch && ch->Ball())
			mode |= 2;
	};
	if (victim->Ball())
	{
		victim->DropBall();
		if (weaponid != WEAPON_GAME) // exclude tee goal
			GameServer()->SendGameMsg(GAMEMSG_CTF_DROP, -1);
		if (killer && killer->GetTeam() != victim->GetPlayer()->GetTeam())
			killer->m_Score++;
		mode |= 1;
	}

	return mode;
	
}

void CGameControllerFB::OnCharacterSpawn(class CCharacter *chr)
{
	// default health
	chr->IncreaseHealth(10);

	if (m_Instagib)
	{
		// give instagib weapon
		chr->GiveWeapon(WEAPON_LASER, -1);
		chr->SetWeapon(WEAPON_LASER);
		chr->DischargeWeapon(g_Config.m_SvfbSpawnWeaponDischarge * g_pData->m_Weapons.m_aId[WEAPON_LASER].m_Firedelay / 100);
	}
	else
	{
		// give default weapons
		chr->GiveWeapon(WEAPON_HAMMER, -1);
		chr->GiveWeapon(WEAPON_GUN, 10);
	}

}

bool CGameControllerFB::HandleGoal (CBall *b, int goal)
{
	
	CPlayer *const p = b->GetCarrier() ? b->GetCarrier()->GetPlayer() : b->m_pLastCarrier;
	const int team = p ? p->GetTeam() : b->m_LastCarrierTeam;
	const bool own_goal = team == goal;
	const bool tee_goal = b->GetCarrier();
	int cid = p->GetCID();
	
	//	update scores
	if (own_goal)
	{
		if (!g_Config.m_SvfbOwngoal)
			return false;
		
		if (p)
		{
			p->m_Score -= g_Config.m_SvfbOwngoalPenalty;
			p->m_NumOwngoals++;
		
			// send warning to user
			char chat_msg[256];
			str_format (chat_msg, sizeof (chat_msg), "!!! Hey %s, you scored an own-goal! Put the ball into the OTHER goal !!!", Server()->ClientName(cid));
			GameServer()->SendChat(-1, CHAT_WHISPER, cid, chat_msg);
		}
		// there's no sound like "wah...wah" ;)
		GameServer()->CreateSoundGlobal(SOUND_NINJA_HIT); //SOUND_PLAYER_PAIN_LONG
	}
	else
	{
		if (p)
			p->m_Score += tee_goal ? g_Config.m_SvfbScorePlayerTee : g_Config.m_SvfbScorePlayerBall;
			
		GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
	}
	m_aTeamscore[goal ^ 1] += tee_goal ? g_Config.m_SvfbScoreTeamTee : g_Config.m_SvfbScoreTeamBall;
		
	// let the tee die if it jumped into goal... or when dumb own-goal
	if (p && (tee_goal || own_goal))
		p->KillCharacter (WEAPON_GAME);
	b->Reset();
	
	// Cut off long nicknames... the message would jut out on screen!
	const int nick_length = 18;
	char nick[64] = {0};
	str_format (nick, sizeof (nick), "%s", p ? Server()->ClientName(cid) : "player left");
	if (str_length (nick) > nick_length)
	{
		nick[nick_length] = 0;
		str_append (nick, "...", sizeof (nick));
	}

	// Build broadcast-message
	char bc_msg[512] = {0};
	str_format (bc_msg, sizeof (bc_msg), "%s team (%s) scored with %s!", (goal ^ 1) == 0 ? "Red" : "Blue",
		nick, tee_goal ? "tee": "ball");

	// wired workaround for multiline broadcast-message center alignment
	// FIXME: can be done smarter!
	if (own_goal)
	{
		char line2[] = "\nPwned... that was an own-goal!";
		const int fillup = 2 * (str_length (bc_msg) - (str_length (line2) - 1));
		str_append (bc_msg, line2, sizeof (bc_msg));

		if (fillup > 0)
		{
			char *cur_pos = bc_msg + str_length (bc_msg);
			for (int l = 0; l < fillup; l++)
				*cur_pos++ = ' ';
			*cur_pos = 0;
		}
	}
	// send everyone the scoring-message
	GameServer()->SendBroadcast(bc_msg, -1);

	// kick unteachable player from server
	if (g_Config.m_SvfbOwngoalKick && p && p->m_NumOwngoals >= g_Config.m_SvfbOwngoalKick)
	{
		char chat_msg[256];
		str_format (chat_msg, sizeof (chat_msg), "%s was automatically kicked. Cause: Scored too many own-goals (%d).",
			Server()->ClientName(cid), g_Config.m_SvfbOwngoalKick);
			
		Server()->Kick(cid,
			"You scored too many own-goals! Please play the "
			"game as it is supposed to be. Thank you!");
		
		GameServer()->SendChat(-1, CHAT_ALL, -1, chat_msg);
	}

	return true;
	
}

void CGameControllerFB::OnPlayerDisconnect (CPlayer *const p)
{
	CCharacter* ch = p->GetCharacter();
	for (int i = 0; i < FB_MAX_BALLS; ++i)
	{
		if (m_apBalls[i])
		{
			if (ch && m_apBalls[i]->GetCarrier() == ch)
				ch->DropBall();
			if (m_apBalls[i]->m_pLastCarrier == p)
				m_apBalls[i]->m_pLastCarrier = NULL;
		}
	}
	IGameController::OnPlayerDisconnect(p);
}

template <typename T> void swap(T& x, T& y)
{
	T z = x;
	x = y;
	y = z;
}

void CGameControllerFB::SetBallColor(CBall *&ball, int team)
{
	/* The flag color cannot be changed by mere assignment to m_Team. When
	 * someone carries a repainted flag, it causes lags due to the way flag
	 * position interpolation is implemented in CItems::RenderFlag in
	 * game/client/components/items.cpp. A hackish workaround is to swap
	 * the flags when the other color is requested, but we should take
	 * special care for the maps where both flags are available */

	if (!ball || ball->GetTeam() == team) return;
	CBall *b = m_apBalls[team];
	if (!b->IsAtStand()) return;

	/* 
	swap(b->m_StandPos, ball->m_StandPos);
	b->m_AtStand            = false;
	b->m_Pos                = ball->m_Pos;
	b->m_Vel                = ball->m_Vel;
	b->m_pCarryingCharacter = ball->m_pCarryingCharacter;
	b->m_pLastCarrier       = ball->m_pLastCarrier;
	b->m_LastCarrierTeam    = ball->m_LastCarrierTeam;
	b->m_DropTick           = ball->m_DropTick;
	b->m_GrabTick           = ball->m_GrabTick;
	b->m_IdleTick           = ball->m_IdleTick;

	ball->Reset();
	ball = b;
	*/
}

void CGameControllerFB::CreateBallGrabSound(int team)
{
	GameServer()->SendGameMsg(GAMEMSG_CTF_GRAB, team, -1);
	/*
	for (int c = 0; c < MAX_CLIENTS; ++c)
	{
		CPlayer *p = GameServer()->m_apPlayers[c];
		if (!p)
			continue;
		GameServer()->CreateSoundGlobal(p->GetTeam() == team ? SOUND_CTF_GRAB_PL : SOUND_CTF_GRAB_EN, p->GetCID());
	}
	*/
}

void CGameControllerFB::GrantBall(CCharacter *ch, CBall *b)
{
	int team = ch->GetPlayer()->GetTeam();
	if (b->IsAtStand())
	{
		m_aTeamscore[team] += g_Config.m_SvfbScoreTeamStand;
		ch->GetPlayer()->m_Score += g_Config.m_SvfbScorePlayerStand;
	}
	if (g_Config.m_SvfbLaserMomentum) 
		SetBallColor(b, team);
	b->Grab(ch);
	ch->Ball() = b;
	ch->BecomeNinja();
	CreateBallGrabSound(team);
}


void CGameControllerFB::Tick()
{
	
	IGameController::Tick();

	if (IsGameRunning())
	{
		
		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			if (!GameServer()->m_apPlayers[i]) continue;

			CCharacter *ch = GameServer()->m_apPlayers[i]->GetCharacter();
			if (!ch || !ch->IsAlive())
				continue;

			// Check for a goal-camper
			if (g_Config.m_SvfbGoalcamp)
			{
				bool in_range = false;
				for (int goal = 0; goal < m_NumGoals; ++goal)
				{
					if (distance (ch->GetPos(), m_apGoalPos[goal]) <
						g_Config.m_SvfbGoalsize * static_cast<float> (g_Config.m_SvfbGoalcampFactor / 10.0f))  //	1.5f
					{
						// if in range, count up 2 ticks
						ch->m_GoalCampTick += 2;
						in_range = true;
						break;
					}
				}
		
				if (!in_range)
				{
					// if out of range, count down just 1 tick again (slower)
					if (ch->m_GoalCampTick)
						ch->m_GoalCampTick--;
				}
				else
				{	
					//	kill goal camping player
					if ((ch->m_GoalCampTick >> 1) >= Server()->TickSpeed() * g_Config.m_SvfbGoalcampTime)
					{
						GameServer()->m_apPlayers[i]->KillCharacter(WEAPON_GAME);
				
						char chat_msg[128];
						str_format (chat_msg, sizeof (chat_msg), "%s was caught camping near a goal...",
							Server()->ClientName(GameServer()->m_apPlayers[i]->GetCID()));
						GameServer()->SendChat(-1, CHAT_ALL, 0, chat_msg);
						continue;
					}
				}
			}

			// anti-camping
			if (g_Config.m_SvfbCampMaxtime)
			{
				if (distance (ch->GetPos(), ch->m_CampPos) <= g_Config.m_SvfbCampThreshold && !ch->Ball())
				{
					// if in range, count up 2 ticks
					ch->m_CampTick += 2;
				}
				else
				{
					// if out of range, count down just 1 tick again (slower)
					if (ch->m_CampTick)
						ch->m_CampTick--;
					else
						ch->m_CampPos = ch->GetPos();
				}
	
				//	kill camping player
				if ((ch->m_CampTick >> 1) >= Server()->TickSpeed () * g_Config.m_SvfbCampMaxtime)
				{
					CPlayer* p = ch->GetPlayer();
					p->KillCharacter(WEAPON_GAME);
					GameServer()->SendChat(p->GetCID(), CHAT_ALL, 0, "I'm a bloody camper");
					continue;
				}
			}
		}
	}

	//	update balls
	for (int bi =  0; bi < FB_MAX_BALLS; ++bi)
	{
		CBall *b = m_apBalls[bi];

		if (!b)
			continue;

		CCharacter* carrier = b->GetCarrier();

		vec2 pos = b->GetPos();
		if (carrier)
			// use carrier position here because balls tend to tick before characters
			pos = carrier->GetPos();

		//	check/handle goal
		bool is_goal = false;
		for (int g = 0; g < m_NumGoals; g++)
		{
			if (distance(pos, m_apGoalPos[g]) < g_Config.m_SvfbGoalsize)
			{
				is_goal = HandleGoal(b, g);
				break;
			}
		}
		if (is_goal)
			continue;

		if (carrier)
		{	//	ball is carried by player
			//	warn ball-carrying player once when he is near his own goal
			CPlayer *p = carrier->GetPlayer();
			int t = p->GetTeam();
			if (g_Config.m_SvfbOwngoalWarn && !p->m_OwngoalWarned && t < m_NumGoals && distance(carrier->GetPos(), m_apGoalPos[t]) < g_Config.m_SvfbGoalsize * 5)
			{
				GameServer()->SendBroadcast(
					"Attention! This is your team's goal\n"
					"you are approaching. You probably\n"
					"want to go to the other side!",
					p->GetCID());

				// play the server-chat sound
				GameServer()->CreateSound(carrier->GetPos(), SOUND_CHAT_SERVER, CmaskOne(p->GetCID()));
				p->m_OwngoalWarned = true;
			}
		}
		else
		{	//	ball is carried by none
			CCharacter *close_characters[MAX_CLIENTS];
			int max_num = GameServer()->m_World.FindEntities(pos, b->ms_PhysSize, (CEntity**)close_characters, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
			int curr_num = 0;
			for (int i = 0; i < max_num; ++i)
			{
				CCharacter *chr = close_characters[i];
				if (chr->IsAlive()
				    && chr->GetPlayer()->GetTeam() != -1
				    // small delay for throwing ball a bit away before fetching again
				    && (b->IsAtStand()
				        || b->m_pLastCarrier != chr->GetPlayer()
				        || Server()->Tick() >= b->GetDropTick() + Server()->TickSpeed() * 0.2f)
				    // don't take flag if already have one (might be useful with two flags)
				    && m_apBalls[bi ^ 1]->GetCarrier() != chr
				    // nothing in between
				    && !GameServer()->Collision()->IntersectLine(pos, chr->GetPos(), NULL, NULL))
					close_characters[curr_num++] = chr;
			}
				
			if (curr_num)
				GrantBall(close_characters[Server()->Tick() % curr_num], b);
		}
	}
}

void CGameControllerFB::Snap(int SnappingClient)
{
	IGameController::Snap(SnappingClient);

	CNetObj_GameDataFlag *pGameDataFlag = static_cast<CNetObj_GameDataFlag *>(Server()->SnapNewItem(NETOBJTYPE_GAMEDATAFLAG, 0, sizeof(CNetObj_GameDataFlag)));
	if(!pGameDataFlag)
		return;

	pGameDataFlag->m_FlagDropTickRed = 0;
	if(m_apBalls[TEAM_RED])
	{
		if(m_apBalls[TEAM_RED]->IsAtStand())
			pGameDataFlag->m_FlagCarrierRed = FLAG_ATSTAND;
		else if(m_apBalls[TEAM_RED]->GetCarrier() && m_apBalls[TEAM_RED]->GetCarrier()->GetPlayer())
			pGameDataFlag->m_FlagCarrierRed = m_apBalls[TEAM_RED]->GetCarrier()->GetPlayer()->GetCID();
		else
		{
			pGameDataFlag->m_FlagCarrierRed = FLAG_TAKEN;
			pGameDataFlag->m_FlagDropTickRed = m_apBalls[TEAM_RED]->GetDropTick();
		}
	}
	else
		pGameDataFlag->m_FlagCarrierRed = FLAG_MISSING;
	pGameDataFlag->m_FlagDropTickBlue = 0;
	if(m_apBalls[TEAM_BLUE])
	{
		if(m_apBalls[TEAM_BLUE]->IsAtStand())
			pGameDataFlag->m_FlagCarrierBlue = FLAG_ATSTAND;
		else if(m_apBalls[TEAM_BLUE]->GetCarrier() && m_apBalls[TEAM_BLUE]->GetCarrier()->GetPlayer())
			pGameDataFlag->m_FlagCarrierBlue = m_apBalls[TEAM_BLUE]->GetCarrier()->GetPlayer()->GetCID();
		else
		{
			pGameDataFlag->m_FlagCarrierBlue = FLAG_TAKEN;
			pGameDataFlag->m_FlagDropTickBlue = m_apBalls[TEAM_BLUE]->GetDropTick();
		}
	}
	else
		pGameDataFlag->m_FlagCarrierBlue = FLAG_MISSING;
}

bool CGameControllerFB::CanSpawn(int Team, vec2 *pOutPos)
{
	CSpawnEval Eval;
	
	// spectators can't spawn
	if(Team == TEAM_SPECTATORS)
		return false;
	
	Eval.m_FriendlyTeam = Team;
	
	// try own team only
	EvaluateSpawnType(&Eval, 1+(Team&1));

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}
