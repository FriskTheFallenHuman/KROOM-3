/*
===========================================================================

KROOM 3 GPL Source Code

This file is part of the KROOM 3 Source Code, originally based
on the Doom 3 with bits and pieces from Doom 3 BFG edition GPL Source Codes both published in 2011 and 2012.

KROOM 3 Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Extra attributions can be found on the CREDITS.txt file

===========================================================================
*/

#ifndef __MSGCHANNEL_H__
#define __MSGCHANNEL_H__


/*
===============================================================================

  Network channel.

  Handles message fragmentation and out of order / duplicate suppression.
  Unreliable messages are not garrenteed to arrive but when they do, they
  arrive in order and without duplicates. Reliable messages always arrive,
  and they also arrive in order without duplicates. Reliable messages piggy
  back on unreliable messages. As such an unreliable message stream is
  required for the reliable messages to be delivered.

===============================================================================
*/

#define	MAX_MESSAGE_SIZE				16384		// max length of a message, which may
// be fragmented into multiple packets
#define CONNECTIONLESS_MESSAGE_ID		-1			// id for connectionless messages
#define CONNECTIONLESS_MESSAGE_ID_MASK	0x7FFF		// value to mask away connectionless message id

#define MAX_MSG_QUEUE_SIZE				16384		// must be a power of 2


class idMsgQueue
{
public:
	idMsgQueue();

	void			Init( int sequence );

	bool			Add( const byte* data, const int size );
	bool			Get( byte* data, int& size );
	int				GetTotalSize() const;
	int				GetSpaceLeft() const;
	int				GetFirst() const
	{
		return first;
	}
	int				GetLast() const
	{
		return last;
	}
	void			CopyToBuffer( byte* buf ) const;

private:
	byte			buffer[MAX_MSG_QUEUE_SIZE];
	int				first;			// sequence number of first message in queue
	int				last;			// sequence number of last message in queue
	int				startIndex;		// index pointing to the first byte of the first message
	int				endIndex;		// index pointing to the first byte after the last message

	void			WriteByte( byte b );
	byte			ReadByte();
	void			WriteShort( int s );
	int				ReadShort();
	void			WriteInt( int l );
	int				ReadInt();
	void			WriteData( const byte* data, const int size );
	void			ReadData( byte* data, const int size );
};


class idMsgChannel
{
public:
	idMsgChannel();

	void			Init( const netadr_t adr, const int id );
	void			Shutdown();
	void			ResetRate();

	// Sets the maximum outgoing rate.
	void			SetMaxOutgoingRate( int rate )
	{
		maxRate = rate;
	}

	// Gets the maximum outgoing rate.
	int				GetMaxOutgoingRate()
	{
		return maxRate;
	}

	// Returns the address of the entity at the other side of the channel.
	netadr_t		GetRemoteAddress() const
	{
		return remoteAddress;
	}

	// Returns the average outgoing rate over the last second.
	int				GetOutgoingRate() const
	{
		return outgoingRateBytes;
	}

	// Returns the average incoming rate over the last second.
	int				GetIncomingRate() const
	{
		return incomingRateBytes;
	}

	// Returns the average outgoing compression ratio over the last second.
	float			GetOutgoingCompression() const
	{
		return outgoingCompression;
	}

	// Returns the average incoming compression ratio over the last second.
	float			GetIncomingCompression() const
	{
		return incomingCompression;
	}

	// Returns the average incoming packet loss over the last 5 seconds.
	float			GetIncomingPacketLoss() const;

	// Returns true if the channel is ready to send new data based on the maximum rate.
	bool			ReadyToSend( const int time ) const;

	// Sends an unreliable message, in order and without duplicates.
	int				SendMessage( idPort& port, const int time, const idBitMsg& msg );

	// Sends the next fragment if the last message was too large to send at once.
	void			SendNextFragment( idPort& port, const int time );

	// Returns true if there are unsent fragments left.
	bool			UnsentFragmentsLeft() const
	{
		return unsentFragments;
	}

	// Processes the incoming message. Returns true when a complete message
	// is ready for further processing. In that case the read pointer of msg
	// points to the first byte ready for reading, and sequence is set to
	// the sequence number of the message.
	bool			Process( const netadr_t from, int time, idBitMsg& msg, int& sequence );

	// Sends a reliable message, in order and without duplicates.
	bool			SendReliableMessage( const idBitMsg& msg );

	// Returns true if a new reliable message is available and stores the message.
	bool			GetReliableMessage( idBitMsg& msg );

	// Removes any pending outgoing or incoming reliable messages.
	void			ClearReliableMessages();

private:
	netadr_t		remoteAddress;	// address of remote host
	int				id;				// our identification used instead of port number
	int				maxRate;		// maximum number of bytes that may go out per second
	idCompressor* 	compressor;		// compressor used for data compression

	// variables to control the outgoing rate
	int				lastSendTime;	// last time data was sent out
	int				lastDataBytes;	// bytes left to send at last send time

	// variables to keep track of the rate
	int				outgoingRateTime;
	int				outgoingRateBytes;
	int				incomingRateTime;
	int				incomingRateBytes;

	// variables to keep track of the compression ratio
	float			outgoingCompression;
	float			incomingCompression;

	// variables to keep track of the incoming packet loss
	float			incomingReceivedPackets;
	float			incomingDroppedPackets;
	int				incomingPacketLossTime;

	// sequencing variables
	int				outgoingSequence;
	int				incomingSequence;

	// outgoing fragment buffer
	bool			unsentFragments;
	int				unsentFragmentStart;
	byte			unsentBuffer[MAX_MESSAGE_SIZE];
	idBitMsg		unsentMsg;

	// incoming fragment assembly buffer
	int				fragmentSequence;
	int				fragmentLength;
	byte			fragmentBuffer[MAX_MESSAGE_SIZE];

	// reliable messages
	idMsgQueue		reliableSend;
	idMsgQueue		reliableReceive;

private:
	void			WriteMessageData( idBitMsg& out, const idBitMsg& msg );
	bool			ReadMessageData( idBitMsg& out, const idBitMsg& msg );

	void			UpdateOutgoingRate( const int time, const int size );
	void			UpdateIncomingRate( const int time, const int size );

	void			UpdatePacketLoss( const int time, const int numReceived, const int numDropped );
};

#endif /* !__MSGCHANNEL_H__ */
