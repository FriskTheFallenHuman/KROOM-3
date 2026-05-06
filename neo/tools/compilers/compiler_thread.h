/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __COMPILER_THREAD_H__
#define __COMPILER_THREAD_H__

/*
===============================================================================

idCompilerThread

Runs any compiler function (dmap, AAS, etc.) on a
dedicated background thread so the main thread stays
responsive. Only one compiler job runs at a time.

===============================================================================
*/

class idCompilerThread : public idSysThread
{
public:
	typedef void ( *CompilerFunc_t )( const idCmdArgs& );

	idCompilerThread();
	~idCompilerThread();

	// Returns false if a job is already running.
	bool            Dispatch( CompilerFunc_t func, const idCmdArgs& args,
							  const char* threadName = "CompilerThread" );

	// Non-blocking poll.
	bool            IsRunning() const;

	// Blocks until the thread finishes.
	void            WaitForCompletion();

	// Last exit code returned by the compiler.
	int             GetResult() const
	{
		return result;
	}

private:
	virtual int     Run() override;

	CompilerFunc_t	func;
	idCmdArgs	args;
	volatile bool	running;
	int	result;
};

extern idCompilerThread g_compilerThread;

#endif	/* !__COMPILER_THREAD_H__ */
