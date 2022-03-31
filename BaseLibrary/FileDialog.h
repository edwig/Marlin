////////////////////////////////////////////////////////////////////////
//
// File: FileDialog.h
//
// Copyright (c) 1998-2022 ir. W.E. Huisman
// All rights reserved
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of 
// this software and associated documentation files (the "Software"), 
// to deal in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
// and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies 
// or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Version number: See SQLComponents.h
//
#pragma once
#include <commdlg.h>

/* VLAGGEN

#define OFN_READONLY                 0x00000001
#define OFN_OVERWRITEPROMPT          0x00000002
#define OFN_HIDEREADONLY             0x00000004
#define OFN_NOCHANGEDIR              0x00000008
#define OFN_SHOWHELP                 0x00000010
#define OFN_ENABLEHOOK               0x00000020
#define OFN_ENABLETEMPLATE           0x00000040
#define OFN_ENABLETEMPLATEHANDLE     0x00000080
#define OFN_NOVALIDATE               0x00000100
#define OFN_ALLOWMULTISELECT         0x00000200
#define OFN_EXTENSIONDIFFERENT       0x00000400
#define OFN_PATHMUSTEXIST            0x00000800
#define OFN_FILEMUSTEXIST            0x00001000
#define OFN_CREATEPROMPT             0x00002000
#define OFN_SHAREAWARE               0x00004000
#define OFN_NOREADONLYRETURN         0x00008000
#define OFN_NOTESTFILECREATE         0x00010000
#define OFN_NONETWORKBUTTON          0x00020000
#define OFN_NOLONGNAMES              0x00040000     // force no long names for 4.x modules
#define OFN_EXPLORER                 0x00080000     // new look commdlg
#define OFN_NODEREFERENCELINKS       0x00100000
#define OFN_LONGNAMES                0x00200000     // force long names for 3.x modules
#define OFN_ENABLEINCLUDENOTIFY      0x00400000     // send include message to callback
#define OFN_ENABLESIZING             0x00800000

Flag Meaning 
OFN_ALLOWMULTISELECT  Specifies that the File Name list box allows multiple selections. 
                      If you also set the OFN_EXPLORER flag, the dialog box uses 
                      the Explorer-style user interface; otherwise, it uses the old-style 
                      user interface. 
                      If the user selects more than one file, the lpstrFile buffer returns 
                      the path to the current directory followed by the filenames of 
                      the selected files. The nFileOffset member is the offset, in bytes 
                      or characters, to the first filename, and the nFileExtension member 
                      is not used. For Explorer-style dialog boxes, the directory and 
                      filename strings are NULL separated, with an extra NULL character 
                      after the last filename. This format enables the Explorer-style 
                      dialogs to return long filenames that include spaces. For old-style 
                      dialog boxes, the directory and filename strings are separated by 
                      spaces and the function uses short filenames for filenames with spaces. 
                      You can use theFindFirstFile function to convert between long and 
                      short filenames. 
                      If you specify a custom template for an old-style dialog box, 
                      the definition of the File Name list box must contain the 
                      LBS_EXTENDEDSEL value. 
 
OFN_CREATEPROMPT      If the user specifies a file that does not exist, this flag causes 
                      the dialog box to prompt the user for permission to create the file. 
                      If the user chooses to create the file, the dialog box closes and 
                      the function returns the specified name; otherwise, the dialog box 
                      remains open. If you use this flag with the OFN_ALLOWMULTISELECT 
                      flag, the dialog box allows the user to specify only one 
                      nonexistent file.  

OFN_ENABLEHOOK        Enables the hook function specified in the lpfnHook member.  

OFN_ENABLESIZING      Windows NT 5.0, Windows 98: Enables the Explorer-style dialog box 
                      to be resized using either the mouse or the keyboard. By default, 
                      the Explorer-style Open and Save As dialog boxes allow the dialog 
                      box to be resized regardless of whether this flag is set. 
                      This flag is necessary only if you provide a hook procedure or 
                      custom template. The old-style dialog box does not permit resizing. 

OFN_ENABLETEMPLATE    Indicates that the lpTemplateName member points to the name of 
                      a dialog template resource in the module identified by the 
                      hInstance member.
                      If the OFN_EXPLORER flag is set, the system uses the specified 
                      template to create a dialog box that is a child of the default 
                      Explorer-style dialog box. If the OFN_EXPLORER flag is not set, 
                      the system uses the template to create an old-style dialog box 
                      that replaces the default dialog box.
 
OFN_ENABLETEMPLATEHANDLE 
                      Indicates that the hInstance member identifies a data block that 
                      contains a preloaded dialog box template. The system ignores the 
                      lpTemplateName if this flag is specified.
                      If the OFN_EXPLORER flag is set, the system uses the specified 
                      template to create a dialog box that is a child of the default 
                      Explorer-style dialog box. If the OFN_EXPLORER flag is not set, 
                      the system uses the template to create an old-style dialog box 
                      that replaces the default dialog box.
 
OFN_EXPLORER          Indicates that any customizations made to the Open or Save As dialog 
                      box use the new Explorer-style customization methods. For more 
                      information, see Explorer-Style Hook Procedures and Explorer-Style 
                      Custom Templates.
                      By default, the Open and Save As dialog boxes use the Explorer-style 
                      user interface regardless of whether this flag is set. 
                      This flag is necessary only if you provide a hook procedure or 
                      custom template, or set the OFN_ALLOWMULTISELECT flag. 
                      If you want the old-style user interface, omit the OFN_EXPLORER 
                      flag and provide a replacement old-style template or 
                      hook procedure. If you want the old style but do not need a custom 
                      template or hook procedure, simply provide a hook procedure that 
                      always returns FALSE.
 
OFN_EXTENSIONDIFFERENT 
                      Specifies that the user typed a filename extension that differs 
                      from the extension specified by lpstrDefExt. The function does not 
                      use this flag if lpstrDefExt is NULL. 

OFN_FILEMUSTEXIST     Specifies that the user can type only names of existing files in 
                      the File Name entry field. If this flag is specified and the user 
                      enters an invalid name, the dialog box procedure displays a warning 
                      in a message box. If this flag is specified, the OFN_PATHMUSTEXIST 
                      flag is also used. 

OFN_HIDEREADONLY      Hides the Read Only check box. 

OFN_LONGNAMES         For old-style dialog boxes, this flag causes the dialog box to 
                      use long filenames. If this flag is not specified, or if the 
                      OFN_ALLOWMULTISELECT flag is also set, old-style dialog boxes use 
                      short filenames (8.3 format) for filenames with spaces. 
                      Explorer-style dialog boxes ignore this flag and always display 
                      long filenames.
 
OFN_NOCHANGEDIR       Restores the current directory to its original value if the user 
                      changed the directory while searching for files. 

OFN_NODEREFERENCELINKS 
                      Directs the dialog box to return the path and filename of the 
                      selected shortcut (.LNK) file. If this value is not given, 
                      the dialog box returns the path and filename of the file referenced 
                      by the shortcut 

OFN_NOLONGNAMES       For old-style dialog boxes, this flag causes the dialog box to use 
                      short filenames (8.3 format). 
                      Explorer-style dialog boxes ignore this flag and always display 
                      long filenames.
 
OFN_NONETWORKBUTTON   Hides and disables the Network button. 

OFN_NOREADONLYRETURN  Specifies that the returned file does not have the Read Only check 
                      box checked and is not in a write-protected directory. 

OFN_NOTESTFILECREATE  Specifies that the file is not created before the dialog box is 
                      closed. This flag should be specified if the application saves the 
                      file on a create-nonmodify network share. When an application 
                      specifies this flag, the library does not check for write protection, 
                      a full disk, an open drive door, or network protection. Applications 
                      using this flag must perform file operations carefully, because a 
                      file cannot be reopened once it is closed. 

OFN_NOVALIDATE        Specifies that the common dialog boxes allow invalid characters 
                      in the returned filename. Typically, the calling application uses 
                      a hook procedure that checks the filename by using the FILEOKSTRING 
                      message. If the text box in the edit control is empty or contains 
                      nothing but spaces, the lists of files and directories are updated. 
                      If the text box in the edit control contains anything else, 
                      nFileOffset and nFileExtension are set to values generated by 
                      parsing the text. No default extension is added to the text, nor is 
                      text copied to the buffer specified by lpstrFileTitle. 
                      If the value specified by nFileOffset is less than zero, the filename 
                      is invalid. Otherwise, the filename is valid, and nFileExtension 
                      and nFileOffset can be used as if the OFN_NOVALIDATE flag had not 
                      been specified. 

OFN_OVERWRITEPROMPT   Causes the Save As dialog box to generate a message box if the 
                      selected file already exists. The user must confirm whether to 
                      overwrite the file. 

OFN_PATHMUSTEXIST     Specifies that the user can type only valid paths and filenames. 
                      If this flag is used and the user types an invalid path and 
                      filename in the File Name entry field, the dialog box function 
                      displays a warning in a message box. 

OFN_READONLY          Causes the Read Only check box to be checked initially when the 
                      dialog box is created. This flag indicates the state of the 
                      Read Only check box when the dialog box is closed. 

OFN_SHAREAWARE        Specifies that if a call to theOpenFile function fails because of 
                      a network sharing violation, the error is ignored and the dialog 
                      box returns the selected filename. 
                      If this flag is not set, the dialog box notifies your hook procedure 
                      when a network sharing violation occurs for the filename specified 
                      by the user. If you set the OFN_EXPLORER flag, the dialog box sends 
                      the CDN_SHAREVIOLATION message to the hook procedure. 
                      If you do not set OFN_EXPLORER, the dialog box sends the 
                      SHAREVISTRING registered message to the hook procedure. 
 
OFN_SHOWHELP          Causes the dialog box to display the Help button. The hwndOwner 
                      member must specify the window to receive the HELPMSGSTRING 
                      registered messages that the dialog box sends when the user clicks 
                      the Help button.
                      An Explorer-style dialog box sends a CDN_HELP notification message 
                      to your hook procedure when the user clicks the Help button. 

*/

class DocFileDialog
{
public:
  DocFileDialog(HWND    p_owner
               ,bool    p_open              // true = open, false = SaveAs
               ,XString p_title             // Title of the dialog
               ,XString p_defext   = ""     // Default extension
               ,XString p_filename = ""     // Default first file
               ,int     p_flags    = 0      // Default flags
               ,XString p_filter  = ""      // Filter for extensions
               ,XString p_direct  = "");    // Default directory to start in
  ~DocFileDialog();

  int     DoModal();
  XString GetChosenFile();

private:
  void FilterString(char *filter);

  bool          m_open;  // open of saveas
  char          m_original[MAX_PATH+1];
  char          m_filename[MAX_PATH+1];
  char          m_filter[1024];
  char          m_title [100];
  char          m_defext[100];
  OPENFILENAME  m_ofn;
};
