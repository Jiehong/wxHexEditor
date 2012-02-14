/***********************************(GPL)********************************
*   wxHexEditor is a hex edit tool for editing massive files in Linux   *
*   Copyright (C) 2010  Erdem U. Altinyurt                              *
*                                                                       *
*   This program is free software; you can redistribute it and/or       *
*   modify it under the terms of the GNU General Public License         *
*   as published by the Free Software Foundation; either version 2      *
*   of the License.                                                     *
*                                                                       *
*   This program is distributed in the hope that it will be useful,     *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of      *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       *
*   GNU General Public License for more details.                        *
*                                                                       *
*   You should have received a copy of the GNU General Public License   *
*   along with this program;                                            *
*   if not, write to the Free Software Foundation, Inc.,                *
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA        *
*                                                                       *
*               home  : www.wxhexeditor.org                             *
*               email : spamjunkeater@gmail.com                         *
*************************************************************************/

#include "HexEditorFrame.h"
#define idDiskDevice 10000
#ifdef  __WXMAC__
	#include <dirent.h>	//for pre 2.9.0 wx releases
#elif defined( __WXMSW__ )
	#include "HDwin.h"
#endif

HexEditorFrame::HexEditorFrame( wxWindow* parent,int id ):
				HexEditorGui( parent, id, wxString(_T("wxHexEditor ")) << _T(_VERSION_STR_ )){
	wxIcon wxHexEditor_ICON ( wxhex_xpm );
	this->SetIcon(wxHexEditor_ICON);
	license=_T("wxHexEditor is a hex editor for HUGE files and devices.\n"
			 "Copyright (C) 2006-2012  Erdem U. Altinyurt\n"
			 "\n"
			 "This program is free software; you can redistribute it and/or\n"
			 "modify it under the terms of the GNU General Public License\n"
			 "as published by the Free Software Foundation version 2 "
			 "of the License."
			 "\n"
			 "This program is distributed in the hope that it will be useful,\n"
			 "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
			 "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
			 "GNU General Public License for more details.\n"
			 "\n"
			 "You should have received a copy of the GNU General Public License\n"
			 "along with this program; if not, write to the Free Software\n"
			 "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
			 "\n"
			 "\n"
			 "mhash used under LGPL licence, Nikos Mavroyanopoulos (c) 1998-2008\n"
			 "Udis86 used under BSD licence, Vivek Thampi (c) 2002-2008\n"
			 "\n"
			 "home:  hexeditor.eu  or  diskeditor.net\n"
			 "email: spamjunkeater@gmail.com\n");

	wxConfigBase *pConfig = wxConfigBase::Get();
	int x = pConfig->Read(_T("ScreenX"), 100),
		 y = pConfig->Read(_T("ScreenY"), 100),
		 w = pConfig->Read(_T("ScreenW"), 600),
		 h = pConfig->Read(_T("ScreenH"), 400);

	// Normalizing for avoid screen disapperaring
	wxSize dsz = wxGetDisplaySize();
	x = x < 0 ? 0 : x < dsz.x ? x : dsz.x - w;
	y = y < 0 ? 0 : y < dsz.y ? y : dsz.y - h;

	// restore frame position and size
	Move(x, y);
	SetClientSize(w, h);


#if defined __WXMAC__ || defined __WXMSW__
	wxArtProvider::Push(new HexEditorArtProvider); //Using similar MacOSX icons. Much more better than wx ones...
#endif
	MyFileHistory = new wxFileHistory( );
	MyFileHistory->UseMenu( menuFileOpenRecent );
	menuFileOpenRecent->Remove( *menuFileOpenRecent->GetMenuItems().begin() ); //Removes "no recent file" message
	MyFileHistory->Load( *wxConfigBase::Get() );

	PrepareAUI();

	MyAUI->Update();
	this->Connect( XORVIEW_EVENT, wxEVT_UPDATE_UI, wxUpdateUIEventHandler( HexEditorFrame::OnUpdateUI ) );
	this->Connect( SELECT_EVENT, wxEVT_UPDATE_UI, wxUpdateUIEventHandler( HexEditorFrame::OnUpdateUI ) );
	this->Connect( UNREDO_EVENT, wxEVT_UPDATE_UI, wxUpdateUIEventHandler( HexEditorFrame::OnUpdateUI ) );
	this->Connect( TAG_CHANGE_EVENT, wxEVT_UPDATE_UI, wxUpdateUIEventHandler( HexEditorFrame::OnUpdateUI ) );
	this->Connect( SEARCH_CHANGE_EVENT, wxEVT_UPDATE_UI, wxUpdateUIEventHandler( HexEditorFrame::OnUpdateUI ) );
	this->Connect( COMPARE_CHANGE_EVENT, wxEVT_UPDATE_UI, wxUpdateUIEventHandler( HexEditorFrame::OnUpdateUI ) );
	this->Connect( wxEVT_CHAR,	wxKeyEventHandler(HexEditorFrame::OnKeyDown),NULL, this);
	this->Connect( wxEVT_ACTIVATE, wxActivateEventHandler(HexEditorFrame::OnActivate),NULL, this );

	this->Connect( idInjection, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( HexEditorFrame::OnMenuEvent ) );
	this->Connect( idBlockSelect, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( HexEditorFrame::OnMenuEvent ) );

//	this->Connect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( HexEditorFrame::OnMenuEvent ) );
	for(int i = 0 ; i < 10 ; i++ )
		this->Connect( MyFileHistory->GetBaseId()+i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( HexEditorFrame::OnMenuEvent ) );

	MyNotebook->Connect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, wxAuiNotebookEventHandler(  HexEditorFrame::OnNotebookTabSelection ), NULL,this );
	MyNotebook->Connect( wxEVT_COMMAND_AUINOTEBOOK_TAB_MIDDLE_UP, wxAuiNotebookEventHandler(  HexEditorFrame::OnNotebookTabClose ), NULL,this );

	bool update_enable = false;
	if ( not wxConfigBase::Get()->Read(_T("UpdateCheck"), &update_enable )){
		update_enable = true;
		wxConfigBase::Get()->Write( _T("UpdateCheck"), update_enable );
		//First Run!
		wxMessageBox( license, _("License Agreement"));
		}


	if( update_enable ){
		double last_chk=0;
		wxConfigBase::Get()->Read(_T("LastUpdateCheckTime"), (&last_chk));
		if( wxDateTime::Now() - wxDateSpan::Week() > wxDateTime( last_chk ) )	//One check for a week enough
			{
			wxConfigBase::Get()->Write(_T("LastUpdateCheckTime"), static_cast< double >( wxDateTime::Now().GetTicks()) );
			VersionChecker vc( wxT("http://wxhexeditor.sourceforge.net/version.php"), wxT(_VERSION_) );
			}
		}
	}

HexEditorFrame::~HexEditorFrame(){
	this->Disconnect( XORVIEW_EVENT, wxEVT_UPDATE_UI, wxUpdateUIEventHandler( HexEditorFrame::OnUpdateUI ) );
	this->Disconnect( SELECT_EVENT, wxEVT_UPDATE_UI, wxUpdateUIEventHandler( HexEditorFrame::OnUpdateUI ) );
	this->Disconnect( UNREDO_EVENT, wxEVT_UPDATE_UI, wxUpdateUIEventHandler( HexEditorFrame::OnUpdateUI ) );
   this->Disconnect( TAG_CHANGE_EVENT, wxEVT_UPDATE_UI, wxUpdateUIEventHandler( HexEditorFrame::OnUpdateUI ) );
   this->Disconnect( SEARCH_CHANGE_EVENT, wxEVT_UPDATE_UI, wxUpdateUIEventHandler( HexEditorFrame::OnUpdateUI ) );
   this->Disconnect( COMPARE_CHANGE_EVENT, wxEVT_UPDATE_UI, wxUpdateUIEventHandler( HexEditorFrame::OnUpdateUI ) );
   this->Disconnect( wxEVT_CHAR,	wxKeyEventHandler(HexEditorFrame::OnKeyDown),NULL, this);
	this->Disconnect( wxEVT_ACTIVATE, wxActivateEventHandler(HexEditorFrame::OnActivate),NULL, this );

	this->Disconnect( idInjection, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( HexEditorFrame::OnMenuEvent ) );
//	this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( HexEditorFrame::OnMenuEvent ) );
	for(int i = 0 ; i < 10 ; i++ )
		this->Disconnect( MyFileHistory->GetBaseId()+i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( HexEditorFrame::OnMenuEvent ) );

	MyNotebook->Disconnect( wxEVT_COMMAND_AUINOTEBOOK_PAGE_CHANGED, wxAuiNotebookEventHandler(  HexEditorFrame::OnNotebookTabSelection ), NULL,this );
	MyNotebook->Disconnect( wxEVT_COMMAND_AUINOTEBOOK_TAB_MIDDLE_UP, wxAuiNotebookEventHandler(  HexEditorFrame::OnNotebookTabClose ), NULL,this );

	wxConfigBase *pConfig = wxConfigBase::Get();
	if ( pConfig == NULL )
		return;
	int x, y, w, h;
	GetClientSize(&w, &h);
	GetPosition(&x, &y);
	pConfig->Write(_T("ScreenX"), (long) x);
	pConfig->Write(_T("ScreenY"), (long) y);
	pConfig->Write(_T("ScreenW"), (long) w);
	pConfig->Write(_T("ScreenH"), (long) h);
	pConfig->Flush();

	MyNotebook->Destroy();
	}

void HexEditorFrame::PrepareAUI( void ){
	MyAUI = new wxAuiManager( this );
	//MyNotebook = new wxAuiNotebook(this,-1);//Creation moved to wxFormDialog
	MyNotebook->SetArtProvider(new wxAuiSimpleTabArt);

//	MyNotebook->SetWindowStyleFlag(wxAUI_NB_TOP|
//											wxAUI_NB_TAB_MOVE|
//											wxAUI_NB_TAB_SPLIT|
//											wxAUI_NB_MIDDLE_CLICK_CLOSE|
//											wxAUI_NB_SCROLL_BUTTONS|
//											wxAUI_NB_WINDOWLIST_BUTTON);

	MyAUI -> AddPane( MyNotebook, wxAuiPaneInfo().
			CaptionVisible(false).
			MinSize(wxSize(150,100)).
			CloseButton(false).
			Center().Layer(1) );

//    wxAuiToolBarItemArray prepend_items;
//    wxAuiToolBarItemArray append_items;
//    wxAuiToolBarItem item;
//    item.SetKind(wxITEM_SEPARATOR);
//    append_items.Add(item);
//    item.SetKind(wxITEM_NORMAL);
//    item.SetId(wxID_ANY);
//    item.SetLabel(_("Customize..."));
//    append_items.Add(item);

#ifdef _WX_AUIBAR_H_
	Toolbar = new wxAuiToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_DEFAULT_STYLE );// wxAUI_TB_OVERFLOW);
#else
	Toolbar = new wxToolBar( this,  wxID_ANY, wxDefaultPosition, wxDefaultSize );
#endif
//	Toolbar->SetToolBitmapSize(wxSize(48,48));

	Toolbar->AddTool(wxID_NEW, _("New File"), wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR), _("New file"));
	Toolbar->AddTool(wxID_OPEN, _("Open File"), wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR), _("Open"));
	Toolbar->AddTool(wxID_SAVE, _("Save File"), wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR), _("Save"));
	Toolbar->AddTool(wxID_SAVEAS, _("SaveAs File"), wxArtProvider::GetBitmap(wxART_FILE_SAVE_AS, wxART_TOOLBAR), _("Save as"));
	Toolbar->AddTool(idClose, _("Close File"), wxArtProvider::GetBitmap(wxART_CROSS_MARK, wxART_TOOLBAR), _("Close"));
	Toolbar->AddSeparator();
	Toolbar->AddTool(wxID_FIND, _("Find"), wxArtProvider::GetBitmap(wxART_FIND, wxART_TOOLBAR), _("Find"));
	Toolbar->AddTool(wxID_REPLACE, _("Replace"), wxArtProvider::GetBitmap(wxART_FIND_AND_REPLACE, wxART_TOOLBAR), _("Find and replace"));
	Toolbar->AddTool(idGotoOffset, _("GoTo"), wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR), _("Go to offset"));
	Toolbar->AddSeparator();
	Toolbar->AddTool(wxID_UNDO, _("Undo"), wxArtProvider::GetBitmap(wxART_UNDO, wxART_TOOLBAR), _("Undo"));
	Toolbar->AddTool(wxID_REDO, _("Redo"), wxArtProvider::GetBitmap(wxART_REDO, wxART_TOOLBAR), _("Redo"));
	Toolbar->AddSeparator();
	Toolbar->AddTool(wxID_COPY, _("Copy Block"), wxArtProvider::GetBitmap(wxART_COPY, wxART_TOOLBAR), _("Copy"));
	Toolbar->AddTool(wxID_PASTE, _("Paste Block"), wxArtProvider::GetBitmap(wxART_PASTE, wxART_TOOLBAR), _("Paste"));
	Toolbar->AddSeparator();
	Toolbar->AddTool(wxID_CUT, _("Cut Block"), wxArtProvider::GetBitmap(wxART_CUT, wxART_TOOLBAR), _("Cuts selected block and copies to clipboard"));
	Toolbar->AddTool(wxID_DELETE, _("Delete Block"), wxArtProvider::GetBitmap(wxART_DELETE, wxART_TOOLBAR), _("Deletes selected block"));
	Toolbar->AddTool(idInsert, _("Insert Block"), wxArtProvider::GetBitmap(wxART_GO_DOWN, wxART_TOOLBAR), _("Insert"));

//  Toolbar->SetCustomOverflowItems(prepend_items, append_items);
   Toolbar->Realize();
	mbar->Check( idToolbar, true );

	MyTagPanel = new TagPanel( this, -1 );
	MyAUI -> AddPane( MyTagPanel, wxAuiPaneInfo().
					Caption(wxT("TagPanel")).
					TopDockable(false).
					BottomDockable(false).
					MinSize(wxSize(70,100)).
					BestSize(wxSize(140,100)).
					Show(false).
					Right().Layer(1) );
	mbar->Check( idTagPanel, true );

	MyDisassemblerPanel = new DisassemblerPanel( this, -1 );
	MyAUI -> AddPane( MyDisassemblerPanel, wxAuiPaneInfo().
					Caption(wxT("Disassembler Panel")).
					TopDockable(false).
					BottomDockable(false).
					MinSize(wxSize(70,100)).
					BestSize(wxSize(140,100)).
					Show(false).
					Right().Layer(1) );
	mbar->Check( idTagPanel, true );

	MySearchPanel = new SearchPanel( this, -1 );
   //Created under OnUpdateUI
   MyAUI -> AddPane( MySearchPanel, wxAuiPaneInfo().
				Caption(_("Search Results")).
				TopDockable(false).
				BottomDockable(false).
				MinSize(wxSize(70,100)).
				BestSize(wxSize(140,100)).
				Show(false).
				Right().Layer(1) );

	MyComparePanel = new ComparePanel( this, -1 );
   //Created under OnUpdateUI
   MyAUI -> AddPane( MyComparePanel, wxAuiPaneInfo().
				Caption(_("Comparison Results")).
				TopDockable(false).
				BottomDockable(false).
				MinSize(wxSize(70,100)).
				BestSize(wxSize(140,100)).
				Show(false).
				Right().Layer(1) );

   MyAUI -> AddPane(Toolbar, wxAuiPaneInfo().
                  Name(_("ToolBar")).Caption(_("Big Toolbar")).
                  ToolbarPane().Top().
                  LeftDockable(false).RightDockable(false));

	MyInfoPanel = new InfoPanel( this, -1 );
	MyAUI -> AddPane( MyInfoPanel, wxAuiPaneInfo().
					Caption(_("InfoPanel")).
					TopDockable(false).
					BottomDockable(false).
					BestSize(wxSize(140,140)).
					//Resizable(false).
					Left().Layer(1) );
	mbar->Check( idInfoPanel, true );

	MyInterpreter = new DataInterpreter( this, -1 );
	MyAUI -> AddPane( MyInterpreter, wxAuiPaneInfo().
					Caption(_("DataInterpreter")).
					TopDockable(false).
					BottomDockable(false).
					BestSize(wxSize(174,218)).
					Resizable(false).
					Left().Layer(1) );
	mbar->Check( idInterpreter, true );

	ActionDisabler();
	MyNotebook->SetDropTarget( new DnDFile( this ) );
	MyInfoPanel->SetDropTarget( new DnDFile( this ) );
	MyTagPanel->SetDropTarget( new DnDFile( this ) );
	MySearchPanel->SetDropTarget( new DnDFile( this ) );
	MyComparePanel->SetDropTarget( new DnDFile( this ) );
	MyInterpreter->SetDropTarget( new DnDFile( this ) );
	MyDisassemblerPanel->SetDropTarget( new DnDFile( this ) );
	Toolbar->SetDropTarget( new DnDFile( this ) );
	}

void HexEditorFrame::ActionEnabler( void ){
	int arr[] = { idFileRO, idFileRW, idFileDW, wxID_SAVE, wxID_SAVEAS, idClose, wxID_FIND, wxID_REPLACE, idGotoOffset, wxID_PASTE };
	for( unsigned i=0 ; i < sizeof(arr)/4 ; i++ ){
		mbar->Enable( arr[i],true );
		Toolbar->EnableTool( arr[i], true );
		}
	MyInterpreter->Enable();
	Toolbar->Refresh();
	}

void HexEditorFrame::ActionDisabler( void ){
	int arr[] = { idFileRO, idFileRW,idFileDW, wxID_SAVE, wxID_SAVEAS, idClose, wxID_FIND, wxID_REPLACE, idInsert, idGotoOffset, wxID_PASTE, wxID_CUT, wxID_DELETE, wxID_COPY, idCopyAs, wxID_UNDO, wxID_REDO, };
	for( unsigned i=0 ; i < sizeof(arr)/4 ; i++ ){
		mbar->Enable( arr[i],false );
		Toolbar->EnableTool( arr[i], false );
		}
	MyInterpreter->Clear();
	MyInterpreter->Disable();
	MyDisassemblerPanel->Clear();
	Toolbar->Refresh();
	}

HexEditor* HexEditorFrame::OpenFile(wxFileName flname){
	HexEditor *x = new HexEditor(MyNotebook, -1, statusBar,	MyInterpreter,	MyInfoPanel, MyTagPanel, MyDisassemblerPanel );
	if(x->FileOpen( flname )){
		MyNotebook->AddPage( x, flname.GetFullName(), true );

		//For loop updates Open Recent Menu properly.
		int found = -1;
		for( unsigned i=0; i < MyFileHistory->GetCount() ; i++)
			if( MyFileHistory->GetHistoryFile( i ) == flname.GetFullPath() )
				found = i;

		if( found != -1 )
				MyFileHistory->RemoveFileFromHistory( found );
		MyFileHistory->AddFileToHistory( flname.GetFullPath() );
		MyFileHistory->Save( *wxConfigBase::Get() );
		ActionEnabler();
		return x;
		}
	else{
		x->Destroy();
		return NULL;
		}
	}

HexEditor* HexEditorFrame::GetActiveHexEditor( void ){
// TODO (death#1#): BUG : MyNotebook = warning RTTI symbol not found for class wxAuiFloatingFrame	int x = MyNotebook->GetSelection();
	return static_cast<HexEditor*>( MyNotebook->GetPage( x ) );
	}

void HexEditorFrame::OnMenuEvent( wxCommandEvent& event ){
	std::cout << "OnMenuEvent: "  << event.GetId() << std::endl;
	if( event.GetId() == wxID_NEW ){	//GetFile Lenght, Save file as, Create file, Open file as RW
		wxString lngt;
		long unsigned int size=0;
		while(1){
			lngt = wxGetTextFromUser( _("Please indicate file size in decimal."),
                                    _("Enter File Size:"));
			if(lngt.IsEmpty()){
				return;
			}
			else if( lngt.ToULong( &size, 10 ) and (size > 0) )// ToULongLong has problems with Windows...
				break;

			wxMessageBox( wxString::Format(_("Wrong input: %d please retry..."),lngt.ToULong( &size, 10 ) ) ,_T("Error!"), wxICON_ERROR, this );
			}
		//Save file
		wxFileDialog* filediag = new wxFileDialog(this,
									_("Choose a file for save as"),
									_(""),
									_(""),
									_("*"),
									wxFD_SAVE|wxFD_OVERWRITE_PROMPT|wxFD_CHANGE_DIR,
									wxDefaultPosition);

		if(wxID_OK == filediag->ShowModal()){
			wxFileName flname(filediag->GetPath());
			//create file
			wxFile crt;
			if( not crt.Create( flname.GetFullPath(), true ) ){
				wxMessageBox( _("File cannot create!") ,_T("Error!"), wxICON_ERROR, this );
				return;
				}
			if( not crt.Open( flname.GetFullPath(), wxFile::read_write ) ){
				wxMessageBox( _("File cannot open!") ,_T("Error!"), wxICON_ERROR, this );
				return;
				}
			crt.Seek( size-1 );
			crt.Write("\0x00", 1);
			crt.Close();
			//Openning the file with text editor.
			OpenFile( flname );
			filediag->Destroy();
			}
		return; // Without this, wxID_NEW retriggers this function again under wxMSW
		}
	else if( event.GetId() == wxID_OPEN ){
		wxFileDialog filediag(this,
									_("Choose a file for editing"),
									_(""),
									_(""),
									_("*"),
									wxFD_FILE_MUST_EXIST|wxFD_OPEN|wxFD_CHANGE_DIR,
									wxDefaultPosition);
		if(wxID_OK == filediag.ShowModal()){
			wxFileName flname(filediag.GetPath());
			OpenFile( flname );
			filediag.Destroy();
			}
		return; // Without this, wxID_OPEN retriggers this function again under wxMSW
		}
	else if( event.GetId() >= MyFileHistory->GetBaseId() and event.GetId() <= MyFileHistory->GetBaseId()+MyFileHistory->GetCount()-1){
		wxString filename = MyFileHistory->GetHistoryFile( event.GetId() - MyFileHistory->GetBaseId() );
		OpenFile( filename );
		}
	else{
		if( MyNotebook->GetPageCount() ){
			HexEditor *MyHexEditor = GetActiveHexEditor();
			if( MyHexEditor != NULL ){
				switch( event.GetId() ){
					//case wxID_OPEN: not handled here!
					case wxID_SAVE:		MyHexEditor->FileSave( false );		break;
					case wxID_SAVEAS:{
						wxFileDialog filediag(this,
															_("Choose a file for save as"),
															_(""),
															_(""),
															_("*"),
															wxFD_SAVE|wxFD_OVERWRITE_PROMPT|wxFD_CHANGE_DIR,
															wxDefaultPosition);
						if(wxID_OK == filediag.ShowModal()){
							if( !MyHexEditor->FileSave( filediag.GetPath() )){
								wxMessageBox( wxString(_("File cannot save as ")).Append( filediag.GetPath() ),_("Error"), wxOK|wxICON_ERROR, this );
								}
							}
						break;
						}
					case idClose:{
						if( MyHexEditor->FileClose() ){
							MyNotebook->DeletePage( MyNotebook->GetSelection() );// delete MyHexEditor; not neccessery, DeletePage also delete this
							if( MyNotebook->GetPageCount() == 0 )
								ActionDisabler();
							return;
							}
						break;
						}
					case wxID_UNDO:		MyHexEditor->DoUndo();					break;
					case wxID_REDO:		MyHexEditor->DoRedo();					break;
					case wxID_COPY:		MyHexEditor->CopySelection();			break;
					case idCopyAs:			MyHexEditor->CopyAsDialog();			break;
					case idSaveAsDump:	MyHexEditor->SaveAsDump();				break;
					case idFillSelection:	MyHexEditor->FillSelection();		break;
					case idBlockSelect:	MyHexEditor->BlockSelect();			break;
					case wxID_CUT:			MyHexEditor->CutSelection();			break;
					case wxID_PASTE:		MyHexEditor->PasteFromClipboard();	break;
					case wxID_DELETE:		MyHexEditor->DeleteSelection();		break;
					case idInsert:			MyHexEditor->InsertBytes();			break;
					//idInjection for Right click Menu Insertion Event
					case idInjection:		MyHexEditor->InsertBytes();			break;
					case wxID_FIND:		MyHexEditor->FindDialog();				break;
					case wxID_REPLACE:	MyHexEditor->ReplaceDialog();			break;
					case idGotoOffset:	MyHexEditor->GotoDialog();				break;
					case idFileRO:{
						MyHexEditor->SetFileAccessMode( FAL::ReadOnly );
						MyInfoPanel->Set( MyHexEditor->GetFileName(), MyHexEditor->FileLength(), MyHexEditor->GetFileAccessModeString(), MyHexEditor->GetFD(), MyHexEditor->FileGetXORKey() );
						break;
						}
					case idFileRW:{
						MyHexEditor->SetFileAccessMode( FAL::ReadWrite );
						MyInfoPanel->Set( MyHexEditor->GetFileName(), MyHexEditor->FileLength(), MyHexEditor->GetFileAccessModeString(), MyHexEditor->GetFD(), MyHexEditor->FileGetXORKey() );
						break;
						}
					case idFileDW:
						if( wxOK == wxMessageBox( _("This mode will write changes every change to file DIRECTLY."),_("Warning!"), wxOK|wxCANCEL|wxICON_WARNING, this, wxCenter ) )
							MyHexEditor->SetFileAccessMode( FAL::DirectWrite );
						MyInfoPanel->Set( MyHexEditor->GetFileName(), MyHexEditor->FileLength(), MyHexEditor->GetFileAccessModeString(), MyHexEditor->GetFD(), MyHexEditor->FileGetXORKey() );
						break;
					case wxID_CLOSE:
						OnQuit( event );
						break;
					default: wxBell();
					}
				}
			}
		return; // Without this, the event retriggers this function again under wxMSW
		}
	event.Skip();
	}

void HexEditorFrame::OnToolsMenu( wxCommandEvent& event ){
#ifdef _DEBUG_
	std::cout << "HexEditorFrame::OnToolsMenu() EventID=" << event.GetId() << std::endl;
#endif
	if( event.GetId() == idXORView )
		GetActiveHexEditor()->FileSetXORKey( event.IsChecked() );
	else if( event.GetId() == idCompare ){
		::CompareDialog *mcd = new CompareDialog( this );
		mcd->ShowModal();
		#ifndef __WXOSX__ // TODO: This leaks memory but OSX magically give error if I Destroy this.. Really Weird. Please help to fix this.
		mcd->Destroy();
		#endif
		}
	else if( event.GetId() == idChecksum){
		::ChecksumDialog *mcd = new ChecksumDialog( this );
		mcd->ShowModal();
		#ifndef __WXOSX__ // TODO: This leaks memory but OSX magically give error if I Destroy this.. Really Weird. Please help to fix this.
		mcd->Destroy();
		#endif
		}
	event.Skip(false);
	}

void HexEditorFrame::OnDevicesMenu( wxCommandEvent& event ){
	if( event.GetId() >= idDiskDevice ){
		int i=event.GetId() - idDiskDevice;
		wxArrayString disks;
#ifdef __WXGTK__ //linux
		if(wxDir::Exists(wxT("/dev/disk/by-id")) ){
			wxDir::GetAllFiles(wxT("/dev/disk/by-id"), &disks );
			}
		else{
			wxDir::GetAllFiles(wxT("/dev"), &disks, wxT("sd*"), wxDIR_FILES );
			wxDir::GetAllFiles(wxT("/dev"), &disks, wxT("hd*"), wxDIR_FILES );
			}
#elif defined( __WXMAC__ )
	#if wxCHECK_VERSION(2, 9, 0) //Problem on wx 2.8.x, returns null.
		wxDir::GetAllFiles(wxT("/dev"), &disks, wxT("disk*"), wxDIR_FILES );
	#else
		DIR *dirp;
			struct dirent *entry;
			if(dirp = opendir("/dev")){
				while(entry = readdir(dirp))
					if( !strncmp(entry->d_name, "disk", 4) )
						disks.Add(wxT("/dev/") + wxString::FromAscii(entry->d_name));
				closedir(dirp);
				}
	#endif
#elif defined( __WXMSW__ )
 		windowsHDD windevs;
 		vector<wchar_t*> DevVector = windevs.getdevicenamevector();
 		for(int i=0; i < DevVector.size();i++)
			disks.Add(wxString(DevVector[i]));
#endif
		disks.Sort();
		OpenFile( wxFileName(disks.Item(i)) );
		}
	else if( event.GetId() == idDeviceRam ){
		//OpenFile( wxFileName( wxT("\\\\.\\PhysicalDrive0") ) );//Just testing
// TODO (death#1#): RAM access with mmap and need DirectWrite Mode
		}
	}

void HexEditorFrame::OnClose( wxCloseEvent& event ){
	wxCommandEvent evt;
	OnQuit( evt );
	event.Skip(false);
	}

void HexEditorFrame::OnQuit( wxCommandEvent& event ){
	HexEditor *MyHexEditor;
	for(;;){
		if( MyNotebook->GetPageCount() ){
			MyHexEditor = static_cast<HexEditor*>(MyNotebook->GetPage( 0 ) );
			if( MyHexEditor != NULL ){
				if( MyHexEditor->FileClose() ){
					MyNotebook->DeletePage( 0 );
					MyHexEditor = NULL;
					}
				else
					break;
				}
			else
				wxLogError(_("Error on quit!"));
			}
		else{
			Destroy();
			//event.Skip();//Makes error on OSX
			return;
			}
		}
	}

void HexEditorFrame::OnViewMenu( wxCommandEvent& event ){
	switch( event.GetId() ){
		case idInterpreter:
			MyAUI->GetPane(MyInterpreter).Show(event.IsChecked());
			break;
		case idInfoPanel:
			MyAUI->GetPane(MyInfoPanel).Show(event.IsChecked());
			break;
		case idToolbar:
			MyAUI->GetPane(Toolbar).Show(event.IsChecked());
			break;
		case idTagPanel:
			MyAUI->GetPane(MyTagPanel).Show(event.IsChecked());
			break;
		case idDisassemblerPanel:
			MyAUI->GetPane(MyDisassemblerPanel).Show(event.IsChecked());
			break;
		default:
			wxBell();
		}
	MyAUI->Update();
	}

void HexEditorFrame::OnAbout( wxCommandEvent& event ){
	wxAboutDialogInfo AllAbout;
	AllAbout.SetName(_T("wxHexEditor"));
	AllAbout.SetVersion( _T(_VERSION_STR_) );
	AllAbout.SetDescription(_("wxHexEditor is a hex editor for HUGE files and devices."));
	AllAbout.SetCopyright(_T("(C) 2006-2012 Erdem U. Altinyurt"));
	AllAbout.AddDeveloper( _T("Erdem U. Altinyurt") );
	AllAbout.AddArtist( _T("Vlad Adrian") );
	AllAbout.SetWebSite( _T("http://www.wxhexeditor.org"));

	AllAbout.SetLicense( license );

	wxAboutBox(AllAbout);
	}

void HexEditorFrame::OnUpdateUI(wxUpdateUIEvent& event){
#ifdef _DEBUG_
	std::cout << "HexEditorFrame::OnUpdateUI(wxUpdateUIEvent& event) ID " << event.GetId() << "\n" ;
#endif
	mbar->Check(idInterpreter, MyInterpreter->IsShown());
	mbar->Check(idInfoPanel, MyInfoPanel->IsShown());
	mbar->Check(idTagPanel, MyTagPanel->IsShown());
	mbar->Check(idDisassemblerPanel, MyDisassemblerPanel->IsShown());
	mbar->Check(idToolbar, Toolbar->IsShown());
	mbar->Check(idXORView, (MyNotebook->GetPageCount() and GetActiveHexEditor()->IsFileUsingXORKey()));
	mbar->Enable(idXORView, MyNotebook->GetPageCount() );
	if(event.GetId() == idDeviceRam ){
		//when updateUI received by Ram Device open event is came, thna needed to update Device List.
#ifdef _DEBUG_
		std::cout << "HexEditorFrame::Ram event :" << event.GetString().ToAscii() << std::endl ;
#endif

		this->Disconnect( wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( HexEditorFrame::OnDevicesMenu ) );
		wxMenuItemList devMen = menuDeviceDisk->GetMenuItems();
		for( wxMenuItemList::iterator it = devMen.begin(); it != devMen.end() ; it++ )
				menuDeviceDisk->Destroy( *it );

		wxArrayString disks;
#ifdef __WXGTK__
		///ls -l /dev/disk/by-id
		//wxExecute(wxT("ls \-1 \\dev\\sd*"), disks, disks);
		if( wxDir::Exists(wxT("/dev/disk/by-id")) ){
			wxDir::GetAllFiles(wxT("/dev/disk/by-id"), &disks );
			}
		else{
			wxDir::GetAllFiles(wxT("/dev"), &disks, wxT("sd*"), wxDIR_FILES );
			wxDir::GetAllFiles(wxT("/dev"), &disks, wxT("hd*"), wxDIR_FILES );
			}
#elif defined( __WXMAC__ )
		//wxDir::GetAllFiles(wxT("/dev"), &disks, wxT("rdisk*"), wxDIR_FILES ); those are "character files". Not eligible for opening in Hex Editor
	#if wxCHECK_VERSION(2, 9, 0) //Problem on wx 2.8.x, returns null.
		wxDir::GetAllFiles(wxT("/dev"), &disks, wxT("disk*"), wxDIR_FILES );
	#else
		DIR *dirp;
			struct dirent *entry;
			if(dirp = opendir("/dev")){
				while(entry = readdir(dirp))
					if( !strncmp(entry->d_name, "disk", 4) )
						//Note, this /dev/ will drop on adding menu for cosmetic!
						disks.Add(wxT("/dev/") + wxString::FromAscii(entry->d_name));
				closedir(dirp);
				}
	#endif

#elif defined( __WXMSW__ )
		windowsHDD windevs;
		vector<wchar_t*> DevVector = windevs.getdevicenamevector();
		for(int i=0; i < DevVector.size();i++)
			disks.Add(wxString(DevVector[i]));
#endif

		//This /dev/ will drop on adding menu for cosmetic!
		for( unsigned i =0 ; i < disks.Count() ; i++)
			disks[i]=disks.Item(i).AfterLast('/');
		disks.Sort();

		wxMenu *nm;
		int last_item=0;
		for( unsigned i =0 ; i < disks.Count() ; i++){
			//Old way...
			//menuDeviceDisk->Append( idDiskDevice+i, disks.Item(i).AfterLast('/'), wxT(""), wxITEM_NORMAL );

			//SubMenu categorization for posix
			#ifndef __WXMSW__
			if( disks.Item(i).StartsWith( disks.Item( last_item ) ) and i not_eq 0 )
				nm->Append( idDiskDevice+i, disks.Item(i), wxT(""), wxITEM_NORMAL );
			else{
				nm=new wxMenu( );
				nm->Append( idDiskDevice+i, disks.Item(i), wxT(""), wxITEM_NORMAL );
				menuDeviceDisk->AppendSubMenu( nm, disks.Item(i) );
				last_item = i;
				}
			#else	//Windows device menu categorization
			if( disks.Item(i).StartsWith( disks.Item( last_item ).BeforeLast('\\') ) and i not_eq 0 )
				nm->Append( idDiskDevice+i, disks.Item(i).AfterLast('\\'), wxT(""), wxITEM_NORMAL );
			else{
				nm=new wxMenu( );
				nm->Append( idDiskDevice+i, disks.Item(i).AfterLast('\\'), wxT(""), wxITEM_NORMAL );
				menuDeviceDisk->AppendSubMenu( nm, disks.Item(i).BeforeLast('\\')+wxT('\\') );
				last_item = i;
				}
			#endif


			this->Connect( idDiskDevice+i, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( HexEditorFrame::OnDevicesMenu ) );
			}
		#ifdef __WXMSW__
			if( disks.Count() == 0 ){
				menuDeviceDisk->Append( idDiskDevice+0, _("You needed administration privileges to see available devices"), wxT(""), wxITEM_NORMAL );
				//menuDeviceDisk->GetMenuItems().Item(1).Toggle();
				}
		#endif
		}

	if( MyNotebook->GetPageCount() ){
		HexEditor *MyHexEditor = GetActiveHexEditor();
		if( MyHexEditor != NULL ){
			switch( MyHexEditor->GetFileAccessMode() ){
				case FAL::ReadOnly :
					mbar->Check(idFileRO, true);
					break;
				case FAL::ReadWrite :
					mbar->Check(idFileRW, true);
					break;
				case FAL::DirectWrite :
					mbar->Check(idFileDW, true);
					break;
				}
			}

		if(event.GetId() == SELECT_EVENT or event.GetId()==XORVIEW_EVENT){
			#ifdef _DEBUG_SELECT_
				std::cout << "HexEditorFrame::Select_Event :" << event.GetString().ToAscii() << std::endl ;
			#endif
			Toolbar->EnableTool( wxID_COPY, event.GetString() == wxT("Selected") );
			mbar->Enable( wxID_COPY, event.GetString() == wxT("Selected") );
			Toolbar->EnableTool( wxID_PASTE, event.GetString() == wxT("NotSelected") );
			mbar->Enable( wxID_PASTE, event.GetString() == wxT("NotSelected") );

			mbar->Enable( idCopyAs, event.GetString() == wxT("Selected") );
			mbar->Enable( idSaveAsDump, event.GetString() == wxT("Selected") );
			mbar->Enable( idFillSelection, event.GetString() == wxT("Selected") );

			if(not GetActiveHexEditor()->IsFileUsingXORKey()){
				Toolbar->EnableTool( wxID_CUT, event.GetString() == wxT("Selected") );
				mbar->Enable( wxID_CUT, event.GetString() == wxT("Selected") );
				Toolbar->EnableTool( wxID_DELETE, event.GetString() == wxT("Selected") );
				mbar->Enable( wxID_DELETE, event.GetString() == wxT("Selected") );
				Toolbar->EnableTool( idInsert, event.GetString() == wxT("NotSelected") );
				mbar->Enable( idInsert, event.GetString() == wxT("NotSelected") );
				}
			else{
				Toolbar->EnableTool( wxID_CUT, false );
				mbar->Enable( wxID_CUT, false );
				Toolbar->EnableTool( wxID_DELETE, false );
				mbar->Enable( wxID_DELETE, false );
				Toolbar->EnableTool( idInsert, false );
				mbar->Enable( idInsert, false);
				}
			Toolbar->Refresh();
			}

		if(event.GetId()==XORVIEW_EVENT){
			int sel = MyNotebook->GetSelection();
			wxString S = MyNotebook->GetPageText(sel);
			wxString XORViewStr = wxT(" (XORView)");
			if( event.GetString()==wxT("Checked") )
				if( not S.EndsWith(XORViewStr) )
					S.Append( XORViewStr );
			if( event.GetString()==wxT("UnChecked") )
				if( S.EndsWith( XORViewStr ))
					S.Remove( S.Len()-XORViewStr.Len() );

			MyNotebook->SetPageText(sel, S);
			}

		if(event.GetId() == UNREDO_EVENT ){
			#ifdef _DEBUG_FILE_
				std::cout << "HexEditorFrame::UNREDO event :" << event.GetString().ToAscii() << std::endl ;
			#endif
			Toolbar->EnableTool( wxID_UNDO, MyHexEditor->IsAvailable_Undo() );
			Toolbar->EnableTool( wxID_REDO, MyHexEditor->IsAvailable_Redo() );
			mbar->Enable( wxID_UNDO, MyHexEditor->IsAvailable_Undo() );
			mbar->Enable( wxID_REDO, MyHexEditor->IsAvailable_Redo() );
			Toolbar->Refresh();
			}

		if(event.GetId() == TAG_CHANGE_EVENT ){
			MyTagPanel->Set( GetActiveHexEditor()->MainTagArray );
			}

		if(event.GetId() == SEARCH_CHANGE_EVENT ){
			MyAUI->GetPane(MySearchPanel).Show(true);
			MyAUI->Update();
			MySearchPanel->Set( GetActiveHexEditor()->HighlightArray );
			}

		if(event.GetId() == COMPARE_CHANGE_EVENT ){
			MyAUI->GetPane(MyComparePanel).Show(true);
			MyAUI->Update();
			MyComparePanel->Set( GetActiveHexEditor()->CompareArray );
			}
		}
	event.Skip();
	}

void HexEditorFrame::OnNotebookTabSelection( wxAuiNotebookEvent& event ){
#ifdef _DEBUG_
	std::cout << "HexEditorFrame::OnNotebookTabSelection( wxAuiNotebookEvent& event ) \n" ;
#endif
	if( MyNotebook->GetPageCount() ){
		HexEditor *MyHexEditor = static_cast<HexEditor*>( MyNotebook->GetPage(  event.GetSelection() ) );
			if( MyHexEditor != NULL ){
				MyHexEditor->UpdateCursorLocation(); //Also updates DataInterpreter
				MyInfoPanel->Set( MyHexEditor->GetFileName(), MyHexEditor->FileLength(), MyHexEditor->GetFileAccessModeString(), MyHexEditor->GetFD(), MyHexEditor->FileGetXORKey() );
				MyTagPanel->Set( MyHexEditor->MainTagArray );
				MySearchPanel->Set( MyHexEditor->HighlightArray );
				MyComparePanel->Set( MyHexEditor->CompareArray );

                //Creating custom UpdateUI event for setting mbar, toolbar...
				wxUpdateUIEvent event;
				if( MyHexEditor->select->IsState( Select::SELECT_FALSE ) )
					event.SetString( wxT("NotSelected") );
				else
					event.SetString( wxT("Selected") );

            event.SetId( SELECT_EVENT );
				OnUpdateUI( event );
            event.SetId( UNREDO_EVENT );
				OnUpdateUI( event );
				}
		}
	event.Skip();
	}

void HexEditorFrame::OnNotebookTabClose( wxAuiNotebookEvent& event ){
#ifdef _DEBUG_
	std::cout << "HexEditorFrame::OnNotebookTabClose( wxAuiNotebookEvent& event ) \n" ;
#endif
	if( MyNotebook->GetPageCount() ){
		HexEditor *MyHexEditor = static_cast<HexEditor*>( MyNotebook->GetPage( event.GetSelection() ) );
		if( MyHexEditor != NULL ){
			if( MyHexEditor->FileClose() ){
				MyNotebook->DeletePage( event.GetSelection() );
				// delete MyHexEditor; not neccessery, DeletePage also delete this
				}
			if( MyNotebook->GetPageCount() == 0 )
				ActionDisabler();
			}
		}
	}

void HexEditorFrame::OnActivate( wxActivateEvent& event ){
#ifdef _DEBUG_MOUSE_
	std::cout << "HexEditorFrame::OnActivate( wxActivateEvent& event ) \n" ;
#endif
	TagHideAll();
	}

void HexEditorFrame::TagHideAll( void ){
	if( MyNotebook->GetPageCount() ){
		HexEditor *MyHexEditor = GetActiveHexEditor();
		if( MyHexEditor != NULL )
			MyHexEditor->TagHideAll();
		}
	}

wxBitmap HexEditorArtProvider::CreateBitmap(const wxArtID& id, const wxArtClient& client, const wxSize& WXUNUSED(size)){
	if ( client == wxART_TOOLBAR ){
#if defined __WXMAC__
		if ( id == wxART_NEW )					return wxGetBitmapFromMemory(osx_new);
		if ( id == wxART_FILE_OPEN )			return wxGetBitmapFromMemory(osx_open);
		if ( id == wxART_FILE_SAVE )			return wxGetBitmapFromMemory(osx_save);
		if ( id == wxART_FILE_SAVE_AS )		return wxGetBitmapFromMemory(osx_save);
		if ( id == wxART_CROSS_MARK )			return wxGetBitmapFromMemory(osx_cancel);
		if ( id == wxART_FIND )					return wxGetBitmapFromMemory(osx_find);
		if ( id == wxART_FIND_AND_REPLACE )	return wxGetBitmapFromMemory(osx_find_and_replace);
		if ( id == wxART_GO_FORWARD )			return wxGetBitmapFromMemory(osx_jump_to);
		if ( id == wxART_UNDO )					return wxGetBitmapFromMemory(osx_undo);
		if ( id == wxART_REDO )					return wxGetBitmapFromMemory(osx_redo);
		if ( id == wxART_COPY )					return wxGetBitmapFromMemory(osx_copy);
		if ( id == wxART_PASTE )				return wxGetBitmapFromMemory(osx_paste);
		if ( id == wxART_CUT )					return wxGetBitmapFromMemory(osx_cut);
		if ( id == wxART_DELETE )				return wxGetBitmapFromMemory(osx_delete);
#elif defined __WXMSW__
		if ( id == wxART_NEW )					return wxGetBitmapFromMemory(win_new);
		if ( id == wxART_FILE_OPEN )			return wxGetBitmapFromMemory(win_open);
		if ( id == wxART_FILE_SAVE )			return wxGetBitmapFromMemory(win_save);
		if ( id == wxART_FILE_SAVE_AS )		return wxGetBitmapFromMemory(win_saveas);
		if ( id == wxART_CROSS_MARK )			return wxGetBitmapFromMemory(win_cancel);
		if ( id == wxART_FIND )					return wxGetBitmapFromMemory(win_find);
		if ( id == wxART_FIND_AND_REPLACE )	return wxGetBitmapFromMemory(win_find_and_replace);
		if ( id == wxART_GO_FORWARD )			return wxGetBitmapFromMemory(win_jump_to);
		if ( id == wxART_UNDO )					return wxGetBitmapFromMemory(win_undo);
		if ( id == wxART_REDO )					return wxGetBitmapFromMemory(win_redo);
		if ( id == wxART_COPY )					return wxGetBitmapFromMemory(win_copy);
		if ( id == wxART_PASTE )				return wxGetBitmapFromMemory(win_paste);
		if ( id == wxART_CUT )					return wxGetBitmapFromMemory(win_cut);
		if ( id == wxART_DELETE )				return wxGetBitmapFromMemory(win_delete);
#endif
		}
	return wxNullBitmap;
	}


bool DnDFile::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames){
	size_t nFiles = filenames.GetCount();
	for ( size_t n = 0; n < nFiles; n++ ) {
		wxFileName myfl( filenames[n] );
		if ( myfl.FileExists() )
			HexFramework->OpenFile( myfl );
		else
			wxMessageBox( wxString(_("Dropped file:\n")).Append( myfl.GetPath() ).Append(_("\ncannot open!")),_("Error"), wxOK|wxICON_ERROR );
		}
	return TRUE;
	}

VersionChecker::VersionChecker( wxString _url, wxString _version, wxWindow *parent, wxWindowID id )
:UpdateDialogGui( parent, id ){
	wxURL url( _url );
	if (url.IsOk()){
		url.GetProtocol().SetTimeout(3);
		wxInputStream *in_stream = url.GetInputStream();
		if( in_stream == NULL || in_stream->GetSize() > 10 ){
			return;	//need for keep safe
			}
		char *bfr = new char[in_stream->GetSize()+1];
		memset(bfr, 0, in_stream->GetSize()+1);
		in_stream->Read(bfr, in_stream->GetSize());
		if( strncmp( bfr, _version.To8BitData(),  _version.Len() ) > 0 ){
			wxString newver = wxString::FromAscii( bfr );
			version_text->SetLabel(wxString::Format( _("New wxHexEditor version %s is available!"), newver.c_str() ));
			wxbtmp_icon->SetBitmap(  wxArtProvider::GetBitmap( wxART_TIP, wxART_MESSAGE_BOX ) );
			Centre();
			Fit();
			wxBell();
			ShowModal();
			}
		delete [] bfr;
		delete in_stream;
		}
	}

void VersionChecker::OnChkDisplay( wxCommandEvent& event ){
	wxConfigBase::Get()->Write( _T("UpdateCheck"), !wxchk_display->GetValue());
	}
