#pragma once

struct NetGameServer_t
{
	// the name and description of this listing, which will be display to the
	// client's server browser
	string name;
	string description;

	// whether or not this is a visible 'public' gameserver; this is only used
	// on the masterserver to determine whether or not to broadcast your
	// listing from there
	bool hidden = true;

	// the level and playlist of the server, which will be display to the
	// client's server browser
	string map;
	string playlist;

	// the address and port of the server, validated and set from the
	// masterserver
	string address;
	int port = NULL;

	// the base64 net key used to decrypt game packets, the client has to
	// install this before issuing a connectionless packet
	string netKey;

	// version identifiers used to check if the gameserver and gameclient are
	// compatible with each other
	unsigned int checksum = NULL;
	string versionId;

	// current amount of players, and the maximum allowed for this gameserver
	int numPlayers = NULL;
	int maxPlayers = NULL;

	// the issue time of this listing
	int64_t timeStamp = -1;
};
