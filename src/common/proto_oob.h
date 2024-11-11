//============ Copyright Valve Corporation, All rights reserved. ==============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#if !defined( PROTO_OOB_H )
#define PROTO_OOB_H
#ifdef _WIN32
#pragma once
#endif

#include "proto_version.h"

#define PORT_RCON           37015 // default RCON port, TCP
#define PORT_SERVER         37015 // Default server port, UDP/TCP
#define PORT_CLIENT         37005 // Default client port, UDP/TCP

// Out of band message id bytes
// Prefixes: S = server, C = client, A = any

#define C2S_CONNECT             'A' // client requests to connect
#define S2C_DISCONNECT          'B' // server requests to disconnect

#define C2S_CHALLENGE           'H' // + challenge value
#define S2C_CHALLENGE           'I' // + challenge value

#define S2C_CONNACCEPT          'J' // special protocol for accepted connections
#define S2C_CONNREJECT          'K' // special protocol for rejected connections

// Generic Ping Request
#define A2A_PING                'L' // respond with an A2A_ACK
#define A2A_ACK                 'M' // general acknowledgment without info

#define S2C_MATCHINFO_UPDATE    'N' // remote match info update

// Data Block Request
#define S2C_DATABLOCK_FRAGMENT  'O' // data block fragment
#define C2S_DATABLOCK_ACK       'P' // data block fragment acknowledgment

// All OOB packet start with this sequence
#define CONNECTIONLESS_HEADER 0xFFFFFFFF

#endif
