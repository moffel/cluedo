#define _CRT_SECURE_NO_WARNINGS
#include "cluedo.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <algorithm>

using namespace Cluedo;

CluedoBase::CluedoBase()
{
	m_CurrentPlayer = -1;
}

CluedoBase::~CluedoBase()
{

}

void CluedoBase::setup( unsigned _seed, int _numPlayers, PlayerHandler** _players )
{
	srand( _seed );

	bool validMap = m_Map.load( "map.txt" );
	assert(validMap);
	if( !validMap )
		return;

	m_NumPlayers	= _numPlayers;
	m_CurrentPlayer = rand() % _numPlayers;

	m_Solution[CARDTYPE_ROOM].type			= CARDTYPE_ROOM;
	m_Solution[CARDTYPE_ROOM].room			= (Room)(rand() % MAX_ROOM);
	m_Solution[CARDTYPE_SUSPECT].type		= CARDTYPE_SUSPECT;
	m_Solution[CARDTYPE_SUSPECT].suspect	= (Suspect)(rand() % MAX_SUSPECT);
	m_Solution[CARDTYPE_WEAPON].type		= CARDTYPE_WEAPON;
	m_Solution[CARDTYPE_WEAPON].weapon		= (Weapon)(rand() % MAX_WEAPON);

	Card cards[MAX_CARDS];
	int numCards = 0;
	
	for( int i = 0; i < MAX_ROOM; ++i )
	{
		if( i != m_Solution[CARDTYPE_ROOM].room )
		{
			cards[numCards].type = CARDTYPE_ROOM;
			cards[numCards++].room = (Room)i;
		}
	}
	for( int i = 0; i < MAX_SUSPECT; ++i )
	{
		if( i != m_Solution[CARDTYPE_SUSPECT].suspect )
		{
			cards[numCards].type = CARDTYPE_SUSPECT;
			cards[numCards++].suspect = (Suspect)i;
		}
	}
	for( int i = 0; i < MAX_WEAPON; ++i )
	{
		if( i != m_Solution[CARDTYPE_WEAPON].weapon )
		{
			cards[numCards].type = CARDTYPE_WEAPON;
			cards[numCards++].weapon = (Weapon)i;
		}
	}

	for( int i = 0; i < _numPlayers; ++i )
	{
		m_Players[i].handler	= _players[i];
		m_Players[i].numCards	= 0;
		m_Players[i].position	= (Room)(rand() % MAX_ROOM);
		m_Players[i].mustMove	= true;
		m_Players[i].active		= true;

		new_avatar:
		m_Players[i].avatar = (Suspect)(rand() % MAX_SUSPECT);
		for( int j = 0; j < i; ++j )
		{
			if( m_Players[j].avatar == m_Players[i].avatar )
				goto new_avatar;
		}
	}

	while( numCards )
	{
		int card = rand() % numCards;
		Player& p = m_Players[numCards%_numPlayers];
		p.cards[p.numCards++] = cards[card];
		cards[card] = cards[--numCards];
	}

	for( int i = 0; i < _numPlayers; ++i )
	{
		m_Players[i].handler->setup( m_Players[i].avatar, i, m_Players[i].numCards, m_Players[i].cards );
	}
}

bool CluedoBase::finished() const
{
	return m_CurrentPlayer == -1;
}

void CluedoBase::tick()
{
	if( finished() )
		return;

	// one player turn
	Player& p = m_Players[m_CurrentPlayer];
	Room room = p.handler->chooseRoom( p.position, p.mustMove );
	assert(room != p.position || !p.mustMove);
	
	p.position = room;
	p.mustMove = true;

	Suspect suspect;
	Weapon weapon;
	p.handler->suggestion( &suspect, &weapon );

	for( int i = 0; i < m_NumPlayers; ++i )
	{
		if( i != m_CurrentPlayer && m_Players[i].avatar == suspect )
		{
			m_Players[i].position = room;
			m_Players[i].mustMove = false;
			break;
		}
	}

	int falseProof = -1;
	Card proof;
	for( int i = 1; i < m_NumPlayers; ++i )
	{
		int id = (m_CurrentPlayer+i)%m_NumPlayers;
		if( m_Players[id].handler->proofFalse( room, suspect, weapon, &proof ) )
		{
			falseProof = id;
			break;
		}
	}

	for( int i = 0; i < m_NumPlayers; ++i )
	{
		bool showCard = falseProof != -1 && i == m_CurrentPlayer;
		m_Players[i].handler->onSuggestion( m_CurrentPlayer, room, suspect, weapon, falseProof, showCard ? &proof : 0 );
	}

	if( p.handler->accusation( &suspect, &weapon ) )
	{
		bool valid =	room == m_Solution[CARDTYPE_ROOM].room &&
						suspect == m_Solution[CARDTYPE_SUSPECT].suspect &&
						weapon == m_Solution[CARDTYPE_WEAPON].weapon;

		for( int i = 0; i < m_NumPlayers; ++i )
			p.handler->onAccusation( m_CurrentPlayer, valid );

		if( valid )
		{
			printf( "player %i wins %i/%i/%i\n\n\n", m_CurrentPlayer, room, suspect, weapon );
			m_CurrentPlayer = -1;
		}
		else
		{
			printf( "%i: false accusation %i/%i/%i\n", m_CurrentPlayer, room, suspect, weapon );
			p.active = false;

			int activePlayers = 0;
			for( int i = 0; i < m_NumPlayers; ++i )
				if( m_Players[i].active )
					activePlayers++;
			if( activePlayers == 1 )
			{
				printf( "player %i wins\n\n\n", m_CurrentPlayer );
				m_CurrentPlayer = -1;
			}
		}
	}

	if( !finished() )
	{
		do {
			++m_CurrentPlayer %= m_NumPlayers;
		} while( !m_Players[m_CurrentPlayer].active );
	}
}

//////////////////////////////////////////////////////////////////////////

Map::Map()
{
	m_Width		= 0;
	m_Height	= 0;
}

Map::~Map()
{
}

bool Map::load( const char* _filename )
{
	if( !m_Map.empty() )
		return false;

	FILE* f = fopen( _filename, "r" );
	if( !f )
		return false;

	std::vector<std::string> lines;
	while( !feof(f) )
	{
		char text[512];
		fgets( text, 511, f );
		text[511] = 0;
		lines.push_back(text);
	}
	fclose(f);

	m_Height = lines.size();
	for( int i = 0; i < m_Height; ++i )
		if( (int)lines[i].length() > m_Width )
			m_Width = lines[i].length();

	m_Map.resize(m_Width*m_Height, MP_NONE);

	for( int x = 0; x < m_Height; ++x )
	{
		const std::string& src = lines[x];
		for( int y = 0; y < m_Width; ++y )
		{
			if( src.length() == y )
				break;

			char c = src[y];
			if( c == '.' )
			{
				m_Map[x*m_Width+y] = MP_WALK;
			}
			else if( c >= '0' && c < '0' + MAX_SUSPECT )
			{
				m_Map[x*m_Width+y] = MAX_ROOM + c - '0';	// player start positions
				m_StartPos[c-'0'] = MapPos(x,y);
			}
			else if( c >= 'a' && c < 'a' + MAX_ROOM )
			{
				m_Map[x*m_Width+y] = c - 'a';				// room door
				m_RoomPos[c-'a'].push_back( MapPos(x,y) );
			}
		}
	}

	return check();
}

bool Map::check() const
{
	for( int i = 0; i < MAX_SUSPECT; ++i )
	{
		if( m_StartPos[i].id == 0 )
		{
			printf( "suspect %i has no start position\n", i );
			return false;
		}
	}

	for( int i = 0; i < MAX_ROOM; ++i )
	{
		if( m_RoomPos[i].empty() )
		{
			printf( "room %i has no entry\n", i );
			return false;
		}
	}

	std::vector<MapPos> path;
	for( int s = 0; s < MAX_SUSPECT; ++s )
	{
		const MapPos* start = startPos((Suspect)s);
		for( int r = 0; r < MAX_ROOM; ++r )
		{
			path.clear();
			if( !findPath( start, start+1, roomPosBegin((Room)r), roomPosEnd((Room)r), path ) )
			{
				printf( "suspect %i can't reach room %i\n", s, r );
				return false;
			}
		}
	}
	return true;
}

struct PathMapPos : MapPos
{
	unsigned char cost;
	unsigned short prev;
};

bool Map::findPath( const MapPos* _start, const MapPos* _startEnd, const MapPos* _target, const MapPos* _targetEnd, std::vector<MapPos>& _path ) const
{
	std::list<PathMapPos> o;
	std::vector<PathMapPos> c;
	c.reserve(256);

	for( const MapPos* it = _start; it != _startEnd; ++it )
	{
		PathMapPos p;
		p.id			= it->id;
		p.cost			= 0;
		p.prev			= ~0;
		o.push_back(p);
	}

	while( !o.empty() )
	{
		PathMapPos p = o.front();
		o.pop_front();

		for( const MapPos* it = _target; it != _targetEnd; ++it )
		{
			if( it->id == p.id )
			{
				_path.reserve(p.cost);
				_path.push_back(p);
				do {

					for( std::vector<PathMapPos>::const_iterator it = c.begin(); it != c.end(); ++it )
					{
						if( (*it).id == p.prev )
						{
							assert( p.cost == (*it).cost + 1 );
							p = *it;
							_path.push_back(p);
							break;
						}
					}
				} while( p.prev != 0xffff );

				std::reverse(_path.begin(),_path.end());
				return true;
			}
		}

		PathMapPos n = p;
		n.cost += 1;
		n.prev = p.id;
		
		for( unsigned i = 0; i < 4; ++i )
			checkMapPos( n, i, o, c );

		c.push_back(p);
	}

	return false;
}

static bool sortMapPosPrio( const PathMapPos& _a, const PathMapPos& _b )
{
	return _a.cost < _b.cost;
}

void Map::checkMapPos( PathMapPos& _p, unsigned _add, std::list<PathMapPos>& _open, std::vector<PathMapPos>& _closed ) const
{
	PathMapPos n = _p;
	switch(_add)
	{
	case 0:	n.x -= 1; break;
	case 1:	n.x += 1; break;
	case 2:	n.y -= 1; break;
	case 3:	n.y += 1; break;
	default: assert(!"invalid");
	}

	if( read(n) == MP_NONE )
		return;

	for( std::vector<PathMapPos>::iterator it = _closed.begin(); it != _closed.end(); ++it )
	{
		if( (*it).id == n.id )
		{
			if( (*it).cost <= n.cost )
			{
				return;
			}
			_closed.erase(it);
			break;
		}
	}
	for( std::list<PathMapPos>::iterator it = _open.begin(); it != _open.end(); ++it )
	{
		if( (*it).id == n.id )
		{
			if( (*it).cost <= n.cost )
			{
				return;
			}
			_open.erase(it);
			break;
		}
	}

	_open.insert( std::lower_bound( _open.begin(), _open.end(), n, &sortMapPosPrio ), n );
}

//////////////////////////////////////////////////////////////////////////

class SimpleAi : public PlayerHandler
{
public:
	void setup( Suspect _avatar, int _id, int _numCards, const Card* _cards )
	{
		m_Cards.insert( m_Cards.end(), _cards, _cards + _numCards );
		m_Known.insert( m_Known.end(), _cards, _cards + _numCards );

		m_FinalSuspect = MAX_SUSPECT;
		m_FinalWeapon = MAX_WEAPON;

		m_Id = _id;
	}

	void destroy()
	{
	}

	Room chooseRoom( Room _currentRoom, bool _mustMove )
	{
		std::vector<Room> rooms;
		for( int i = 0; i < MAX_ROOM; ++i )
			if( !_mustMove || i != _currentRoom )
				rooms.push_back( (Room)i );

		for( std::vector<Card>::const_iterator it = m_Known.begin(); it != m_Known.end(); ++it )
			if( (*it).type == CARDTYPE_ROOM )
			{
				for( std::vector<Room>::iterator r = rooms.begin(); r != rooms.end(); ++r )
					if( *r == it->room )
					{
						rooms.erase(r);
						break;
					}
			}

		if( rooms.empty() )
		{
			int r = rand() % (MAX_ROOM-1);
			if( r == _currentRoom )
				r++;
			m_FinalRoom = false;
			return (Room)r;
		}
		m_FinalRoom = true;
		return (Room)rooms[rand() % rooms.size()];
	}

	void suggestion( Suspect* _suspect, Weapon* _weapon )
	{
new_suspect:
		*_suspect = (Suspect)(rand() % MAX_SUSPECT);
		for( std::vector<Card>::const_iterator it = m_Known.begin(); it != m_Known.end(); ++it )
			if( (*it).type == CARDTYPE_SUSPECT && (*it).suspect == *_suspect )
				goto new_suspect;
new_weapon:
		*_weapon = (Weapon)(rand() % MAX_WEAPON);
		for( std::vector<Card>::const_iterator it = m_Known.begin(); it != m_Known.end(); ++it )
			if( (*it).type == CARDTYPE_WEAPON && (*it).weapon == *_weapon )
				goto new_weapon;
	}

	void onSuggestion( int _player, Room _room, Suspect _suspect, Weapon _weapon, int _proofPlayer /* = -1 */, Card* _proof /* = 0 */ )
	{
		if( _player == m_Id )
		{
			printf( "%i: suggestion %i/%i/%i ", m_Id, _room, _suspect, _weapon );
			if( _proofPlayer == -1 )
				printf( "possible\n" );
			else
				printf( "proofed false by %i\n", _proofPlayer );
		}

		if( _player == m_Id && _proofPlayer == -1 )
		{
			m_FinalSuspect = _suspect;
			m_FinalWeapon = _weapon;
		}
		
		if( _proof )
		{
			m_Known.push_back( *_proof );
		}
	}

	bool accusation( Suspect* _suspect, Weapon* _weapon )
	{
		if( m_FinalSuspect != MAX_SUSPECT && m_FinalRoom )
		{
			*_suspect = m_FinalSuspect;
			*_weapon = m_FinalWeapon;
			return true;
		}
		return false;
	}

	bool proofFalse( Room _room, Suspect _suspect, Weapon _weapon, Card* _card )
	{
		for( std::vector<Card>::const_iterator it = m_Cards.begin(); it != m_Cards.end(); ++it )
		{
			*_card = *it;
			if(	( it->type == CARDTYPE_ROOM && _room == it->room ) ||
				( it->type == CARDTYPE_SUSPECT && _suspect == it->suspect ) ||
				( it->type == CARDTYPE_WEAPON && _weapon == it->weapon ) )
				return true;
		}
		return false;
	}

private:
	int					m_Id;
	std::vector<Card>	m_Cards;
	std::vector<Card>	m_Known;

	bool				m_FinalRoom;
	Suspect				m_FinalSuspect;
	Weapon				m_FinalWeapon;
};

void main()
{
	unsigned seed = (unsigned)time(NULL);
	while(true)
	{
		SimpleAi ai[3];
		PlayerHandler* handler[3] = {&ai[0], &ai[1], &ai[2]};

		CluedoBase game;
		game.setup( seed++, 3, handler );

		while(!game.finished())
			game.tick();
	}

}
