#pragma once

XString GetExeFile();
XString GetExePath();
void    TerminateWithoutCleanup(int p_exitcode);
void    CheckExePath(XString p_runtimer,XString p_productName);
