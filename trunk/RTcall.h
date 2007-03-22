#pragma once

HANDLE SpawnAndRedirect( LPCTSTR commandLine, 
						 HANDLE *hStdOutputReadPipe, 
						 LPCTSTR lpCurrentDirectory );
