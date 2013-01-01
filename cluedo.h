#pragma once
#include <vector>
#include <list>

struct PathMapPos;

namespace Cluedo
{
	enum Room
	{
		HALL, 
		LOUNGE,
		DINING_ROOM,
		KITCHEN,
		BALLROOM,
		CONSERVATORY,
		BILLARD_ROOM,
		LIBRARY,
		STUDY,
		MAX_ROOM
	};
	
	enum Suspect
	{
		COLONEL_MUSTARD,
		PROFESSOR_PLUM,
		REVERENT_GREEN,
		MRS_PEACOCK,
		MISS_SCARLETT,
		MRS_WHITE,
		MAX_SUSPECT
	};
	
	enum Weapon
	{
		DAGGER,
		CANDLESTICK,
		REVOLVER,
		ROPE,
		LEAD_PIPE,
		SPANNER,
		MAX_WEAPON
	};
	
	enum CardType
	{
		CARDTYPE_ROOM,
		CARDTYPE_SUSPECT,
		CARDTYPE_WEAPON
	};

	enum 
	{
		MAX_CARDS	= MAX_ROOM + MAX_SUSPECT + MAX_WEAPON,
		PLAYERS_MIN	= 3,
		PLAYERS_MAX = MAX_SUSPECT,
	};

	struct Card
	{
		CardType	type;
		union {
			Room	room;
			Suspect	suspect;
			Weapon	weapon;
		};
	};

	enum MapPoint
	{
		MP_HALL, 
		MP_LOUNGE,
		MP_DINING_ROOM,
		MP_KITCHEN,
		MP_BALLROOM,
		MP_CONSERVATORY,
		MP_BILLARD_ROOM,
		MP_LIBRARY,
		MP_STUDY,

		MP_COLONEL_MUSTARD,
		MP_PROFESSOR_PLUM,
		MP_REVERENT_GREEN,
		MP_MRS_PEACOCK,
		MP_MISS_SCARLETT,
		MP_MRS_WHITE,

		MP_WALK,
		MP_NONE
	};

	struct MapPos
	{
		MapPos( unsigned char _x = 0, unsigned char _y = 0 ) : x(_x), y(_y) {}

		bool operator == ( const MapPos& _o ) const { return id == _o.id; }
		bool operator < ( const MapPos& _o ) const	{ return id < _o.id; }

		union {
			struct {
				unsigned char x;
				unsigned char y;
			};
			unsigned short id;
		};
	};

	class Map
	{
	public:
		Map();
		~Map();

		bool load( const char* _filename );
		bool check() const;

		MapPoint read( MapPos _pos ) const 
		{
			if( _pos.x < m_Height && _pos.y < m_Width )
				return (MapPoint)m_Map[_pos.x*m_Width+_pos.y];
			return MP_NONE;
		}

		void write( MapPos _pos, MapPoint _val )
		{
			if( _pos.x < m_Height && _pos.y < m_Width )
				m_Map[_pos.x*m_Width+_pos.y] = _val;
		}

		bool findPath( const MapPos* _start, const MapPos* _startEnd, const MapPos* _target, const MapPos* _targetEnd, std::vector<MapPos>& _path ) const;

		const MapPos* startPos( Suspect _suspect ) const	{ return m_StartPos + _suspect; }
		const MapPos* roomPosBegin( Room _room ) const		{ return &*m_RoomPos[_room].begin(); }
		const MapPos* roomPosEnd( Room _room ) const		{ return roomPosBegin(_room) + m_RoomPos[_room].size(); }

	private:
		void checkMapPos( PathMapPos& _p, unsigned _add, std::list<PathMapPos>& _open, std::vector<PathMapPos>& _closed ) const;

		unsigned short		m_Width;
		unsigned short		m_Height;
		std::vector<char>	m_Map;
		std::vector<MapPos>	m_RoomPos[MAX_ROOM];
		MapPos				m_StartPos[MAX_SUSPECT];
	};

	class PlayerHandler
	{
	public:
		virtual void setup( Suspect _avatar, int _id, int _numCards, const Card* _cards ) = 0;
		virtual void destroy() = 0;

		virtual Room chooseRoom( Room _currentRoom, bool _mustMove ) = 0;
		virtual void suggestion( Suspect* _suspect, Weapon* _weapon ) = 0;
		virtual bool accusation( Suspect* _suspect, Weapon* _weapon ) = 0;	// return false to not make an accusation
		virtual bool proofFalse( Room _room, Suspect _suspect, Weapon _weapon, Card* _card ) = 0;

		virtual void onSuggestion( int _player, Room _room, Suspect _suspect, Weapon _weapon, int _proofPlayer = -1, Card* _proof = 0 )	{};
		virtual void onAccusation( int _player, bool _valid )	{};

	};

	struct Player
	{
		PlayerHandler*	handler;
		int				numCards;
		Card			cards[MAX_CARDS/PLAYERS_MIN];

		Suspect			avatar;
		Room			position;
		bool			mustMove;
		bool			active;
	};

	class CluedoBase
	{
	public:
		CluedoBase();
		~CluedoBase();

		virtual void setup( unsigned _seed, int _numPlayers, PlayerHandler** _players );

		virtual bool finished() const;
		virtual void tick();

	private:
		int		m_CurrentPlayer;
		int		m_NumPlayers;
		Player	m_Players[PLAYERS_MAX];
		Card	m_Solution[3];

		Map		m_Map;
	};
}

