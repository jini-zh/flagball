/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <generated/server_data.h>
#include <game/server/gamecontext.h>

#include "character.h"
#include "laser.h"
#include <game/server/gamemodes/fb.h>
#include <engine/shared/config.h>

CLaser::CLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER, Pos)
{
	m_Owner = Owner;
	m_Energy = StartEnergy;
	m_Dir = Direction;
	m_Bounces = 0;
	m_EvalTick = 0;
	GameWorld()->InsertEntity(this);
	DoBounce();
}

void CLaser::DoBounce()
{
	m_EvalTick = Server()->Tick();

	if(m_Energy < 0)
	{
		GameServer()->m_World.DestroyEntity(this);
		return;
	}

	// To is the last point in range
	vec2 To = m_Pos + m_Dir * m_Energy;

	// Find a wall in range. To is the wall position
	bool Wall = GameServer()->Collision()->IntersectLine(m_Pos, To, 0x0, &To);

	// Find a character in front of the wall. To is the character position
	CCharacter *pSelf = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *pHit = GameServer()->m_World.IntersectCharacter(m_Pos, To, 0.f, To, pSelf);

	// Handle balls in between From and To
	if(g_Config.m_SvfbLaserMomentum != 0)
	{
		CGameControllerFB *Controller = dynamic_cast<CGameControllerFB *>(GameServer()->m_pController);
		if(Controller)
			for(int i = 0; i < 2; ++i)
			{
				CBall *Ball = Controller->m_apBalls[i];

				if (Ball && !Ball->GetCarrier() && distance(Ball->GetPos() + vec2(0, -38), closest_point_on_line(m_Pos, To, Ball->GetPos())) < g_Config.m_SvfbBallRadius)
					Ball->Hit(GameServer()->m_apPlayers[m_Owner], m_Dir);
			}
	}

	m_From = m_Pos;
	m_Pos = To;

	// Hit the target character
	if(pHit)
	{
		m_Energy = -1;
		pHit->TakeDamage(vec2(0.f, 0.f), m_Dir, g_pData->m_Weapons.m_aId[WEAPON_LASER].m_Damage, m_Owner, WEAPON_LASER);
		return;
	};

	// Bounce off of the wall
	if(Wall)
	{
		vec2 TempPos = m_Pos;
		vec2 TempDir = m_Dir * 4.0f;

		GameServer()->Collision()->MovePoint(&TempPos, &TempDir, 1.0f, 0);
		m_Pos = TempPos;
		m_Dir = normalize(TempDir);

		m_Energy -= distance(m_From, m_Pos) + GameServer()->Tuning()->m_LaserBounceCost;
		m_Bounces++;

		if(m_Bounces > GameServer()->Tuning()->m_LaserBounceNum)
			m_Energy = -1;

		GameServer()->CreateSound(m_Pos, SOUND_LASER_BOUNCE);
		return;
	}

	// Dissipate
	m_Energy = -1;
}

void CLaser::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CLaser::Tick()
{
	if(Server()->Tick() > m_EvalTick+(Server()->TickSpeed()*GameServer()->Tuning()->m_LaserBounceDelay)/1000.0f)
		DoBounce();
}

void CLaser::TickPaused()
{
	++m_EvalTick;
}

void CLaser::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient) && NetworkClipped(SnappingClient, m_From))
		return;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, GetID(), sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_From.x;
	pObj->m_FromY = (int)m_From.y;
	pObj->m_StartTick = m_EvalTick;
}
