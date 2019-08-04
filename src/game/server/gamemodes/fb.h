#ifndef GAME_SERVER_GAMEMODES_FB_H
#define GAME_SERVER_GAMEMODES_FB_H

#include <game/server/gamecontroller.h>
#include <game/server/entities/flag.h>

enum {
	FB_MAX_GOALS = 2,
	FB_MAX_BALLS = 2
};

class CBall: public CFlag
{
public:
	CPlayer* m_pLastCarrier;
	int m_LastCarrierTeam;
	int m_IdleTick;

	CBall(CGameWorld *pGameWorld, int Team, vec2 Pos);

	virtual void Reset();
	virtual void TickDefered();

	void Hit(CPlayer *Player, vec2 Direction);
	void Release(vec2 Velocity);
};

class CGameControllerFB : public IGameController
{
public:
	class CBall *m_apBalls[FB_MAX_BALLS];
	vec2 m_apGoalPos[FB_MAX_GOALS];
	int m_NumGoals;
	bool m_Instagib;

	CGameControllerFB(class CGameContext *pGameServer);
	virtual void Tick();
	virtual void Snap(int SnappingClient);
	
	virtual bool OnEntity (int index, vec2 pos);
	virtual int OnCharacterDeath (class CCharacter *victim, class CPlayer *killer, int weapon);
	virtual void OnCharacterSpawn (class CCharacter *chr);
	virtual void CreateBallGrabSound(int team);
	virtual void GrantBall(CCharacter *, CBall *);
	virtual void SetBallColor(CBall *&, int team);
	virtual bool HandleGoal (CBall *, int goal);
	virtual void OnPlayerDisconnect (CPlayer *const p);
        virtual bool CanSpawn(int Team, vec2 *pOutPos);
        bool DoWincheckMatch();
};

#endif
