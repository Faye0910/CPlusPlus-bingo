﻿//---------------------------------------------------------------------------

#include <vcl.h>
#include<time.h>
#pragma hdrstop
#include "Unit1.h"
#define CLIENT_MAX 5      //client max
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;
String strHost;
bool blnServer;  //TURE SERVER
String newClient;
String ServerName;
String ClientHostName="";
bool fgConnectState;
bool click=true;
String str="";
int line = 0;
int player=0;
int BB=0;
TStringList *list = new TStringList();
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm(Owner)
{
	time_t t;
 srand((unsigned)time(&t));
}
void __fastcall TForm1::FormCreate(TObject *Sender)
{
fgConnectState=false;
disconnect1->Enabled=false;
Form1->Caption="Local host"+IdIPWatch1->LocalIP();
for(int i=0;i<5;i++)
		{
			for(int y=0;y<5;y++)
			{
				buttons[i][y]=NULL;
			}
		}

}
void __fastcall TForm1::Listen1Click(TObject *Sender)
{
   PACKET pkg;
   String strpkg;
   disconnect1->Enabled=true;
   Form1->Caption="I am Server"+IdIPWatch1->LocalIP();
   Listen1->Checked = !Listen1->Checked ;
   if(Listen1->Checked)
   {
	  ClientSocket1->Active=false;
		try{
			 ServerSocket1->Active=true;
		}
		catch(...)
		{
			Listen1->Checked=false;
			ShowMessage("Be a server failure.");
			return;
		}
		ServerName="Server-"+IdIPWatch1->LocalIP();
		StatusBar1->SimpleText="Status:Chat Server Listening...";
		Connect1->Enabled=false;
		Memo1->Lines->Add("Server:"+ServerName+",socket:"+
		IntToStr(ServerSocket1->Socket->SocketHandle));
   }
   else
   {
	  if(ServerSocket1->Active)
	  {
		  pkg.ID="PON";
		  pkg.MainFunc=1;
		  pkg.SubFunc=2;
		  pkg.MsgLength=0;
		  pkg.Msg="";
		  pkg.separation="@";
		  pkg.EndSymbol="#";
		  pkg.Host=ServerName;

		  strpkg=AssemblePacket(pkg);
		  for(int i=0;i<ServerSocket1->Socket->ActiveConnections;i++)
		  {
			  ServerSocket1->Socket->Connections[i]->SendText(strpkg);
		  }
		  ServerSocket1->Active=false;

	  Connect1->Enabled=true;
	  StatusBar1->SimpleText="Status:Chat Server Close";
	  }
   }
   blnServer=true;
   Form1->Tag=0;
}//--------------------------------------------------------------------------
String TForm1::AssemblePacket(PACKET pkg)
{
	String str="",tmp;

	//-------------檔頭--------------
	if(pkg.ID.Length()!=3)
	{
		ShowMessage("檔頭錯誤");
		return NULL;
	}
	else
		str+=pkg.ID;
	//-------------Host----------------
	if(pkg.Host.Length()==0)
	{
		ShowMessage("Host錯誤");
		return NULL;
	}
	else
	{
		str+=pkg.Host;
		str+=pkg.separation;
	}
	//----------MainFunction--------------
	str+=pkg.MainFunc;
	//----------SubFunction--------------
	str+=pkg.SubFunc;
	//-----------mdg length-------------
	tmp=IntToStr(pkg.MsgLength);
	if(tmp.Length()<1)
	{
		ShowMessage("msg長度記錄錯誤");
		return NULL;
	}
	else
	{
		for(int i=0;i<3-tmp.Length(); i++)
				str+="0";

		str+=tmp;
	}
	//------------message--------------------
	if(pkg.MsgLength != pkg.Msg.Length())
	{
		ShowMessage("msg長度記錄錯誤");
		return NULL;
	}
	else
	{
	  str+=pkg.Msg;
	  str+=pkg.EndSymbol;
	}

	return str;
}
void __fastcall TForm1::Connect1Click(TObject *Sender)
{
   Form1->Caption="I am client";

	if(ClientSocket1->Active)
	   ClientSocket1->Active=false;

	   strHost="127.0.0.1";   //可改成輸入方式
	if(InputQuery("Chat Connection","Chat Server IP:",strHost))
	{
	   if(strHost.Length()>0)
		{
		  ClientSocket1->Host=strHost;
		  ClientSocket1->Active=true;
		  Listen1->Checked=false;
		  blnServer=false;
		}
	}


}
void __fastcall TForm1::disconnect1Click(TObject *Sender)
{
   PACKET pkg;
   String strPkg;

   if(blnServer)
	return;

	pkg.ID="PON";
	pkg.MainFunc=1;
	pkg.SubFunc=2;
	pkg.MsgLength=0;
	pkg.Msg="";
	pkg.separation="@";
	pkg.EndSymbol="#";
	pkg.Host=ClientHostName;

	strPkg=AssemblePacket(pkg);

	ClientSocket1->Socket->SendText(strPkg);
	ClientSocket1->Active=false;
	Listen1->Checked=false;
	Connect1->Checked=false;
	disconnect1->Checked=false;
	fgConnectState=false;
	StatusBar1->SimpleText="Status:Disconnect" ;
	Form1->Caption="";
	del();
}
void __fastcall TForm1::Timer1Timer(TObject *Sender)
{
	Timer1->Enabled=false;
	ClientSocket1->Active=false;
}
void __fastcall TForm1::ClientSocket1Connect(TObject *Sender, TCustomWinSocket *Socket)
{
   Timer1->Enabled=false;

   StatusBar1->SimpleText="Status:Connect to "+ Socket->RemoteHost ;
   fgConnectState=true;
   Memo2->Clear();
   Memo2->Visible=true;

   Listen1->Enabled=false;
   Connect1->Enabled=false;
   disconnect1->Enabled=true;

   ClientSocket1->ClientType=ctNonBlocking;
}
void __fastcall TForm1::ClientSocket1Disconnect(TObject *Sender, TCustomWinSocket *Socket)
{
	Timer1->Enabled=false;
	StatusBar1->SimpleText="Server disconnected";
	fgConnectState=false;

	Listen1->Enabled=true;
	Connect1->Enabled=true;
	disconnect1->Checked=false;

	ShowMessage("斷線了");
	del();
	Button1->Visible=true;
}
void __fastcall TForm1::ClientSocket1Read(TObject *Sender, TCustomWinSocket *Socket)
{
	pPACKET pPkg;
	String strPacket;

	if(!fgConnectState)
		return;

	strPacket=Socket->ReceiveText();
	Memo1->Lines->Add(strPacket);

	pPkg=DisassemblePacket(strPacket);

	if(pPkg==NULL)
	{
		return;
	}
	ParsePacket(pPkg);
}
pPACKET TForm1::DisassemblePacket(String strPkg)
{
   PACKET tmpPkg;
   pPACKET pPkg;
   int separator;
   String strTmp;

   if(strPkg[strPkg.Length()]!='#')
   {
   Memo2->Lines->Add("Packet error 1");
   return NULL;
   }

   if(strPkg.Pos("@")<=0)
   {
   Memo2->Lines->Add("Packet error 2");
   return NULL;
   }

   if(strPkg.SubString(1,3)!="PON")
   {
   Memo2->Lines->Add("Packet error 3");
   return NULL;
   }

   //---------------------------------------
   tmpPkg.ID=strPkg.SubString(1,3);

   separator= strPkg.Pos("@");
   tmpPkg.Host=strPkg.SubString(4,separator-4);
   tmpPkg.separation=strPkg.SubString(separator,1);

   tmpPkg.MainFunc=strPkg.SubString(separator+1,1);
   tmpPkg.SubFunc=strPkg.SubString(separator+2,1);

   //-----------------------------------------
   strTmp=strPkg.SubString(separator+3,3);
   try
   {
	tmpPkg.MsgLength=StrToInt(strTmp);
   }
   catch(...)
   {
   Memo2->Lines->Add("Packet error 4");
   return NULL;
   }

   if(tmpPkg.MsgLength<0)
   {
   Memo2->Lines->Add("Packet error 5");
   return NULL;
   }

   strTmp=strPkg.SubString(separator+6,strPkg.Length()-separator-6);
   if(strTmp.Length()!=tmpPkg.MsgLength)
   {
   Memo2->Lines->Add("Packet error 6");
   return NULL;
   }
   else
	tmpPkg.Msg=strTmp;

   for (int i = 0; i < 5; i++)
			{
				for (int j = 0; j < 5; j++)
				{

					if (buttons[i][j]->Caption==tmpPkg.Msg)
					{
						click1[i][j] = 1;
						buttons[i][j]->Enabled=false;
					}

				}

            }

   pPkg=new PACKET;
   pPkg->ID=tmpPkg.ID;
   pPkg->Host=tmpPkg.Host;
   pPkg->separation=tmpPkg.separation;
   pPkg->MainFunc=tmpPkg.MainFunc;
   pPkg->SubFunc=tmpPkg.SubFunc;
   pPkg->MsgLength=tmpPkg.MsgLength;
   pPkg->Msg=tmpPkg.Msg;
   pPkg->EndSymbol=tmpPkg.EndSymbol;

   return pPkg;

}
void __fastcall TForm1::Exit1Click(TObject *Sender)
{
	ServerSocket1->Close();
	ClientSocket1->Close();
	Close();
}
void __fastcall TForm1::ClientSocket1Error(TObject *Sender, TCustomWinSocket *Socket,
		  TErrorEvent ErrorEvent, int &ErrorCode)
{
	Memo2->Lines->Add("Error:"+strHost);
	ErrorCode=0;
}
void TForm1::ParsePacket (pPACKET pPkg)
{
	int mFunc,sFunc;
	String str,strTmp;
	mFunc=StrToInt(pPkg->MainFunc);
	sFunc=StrToInt(pPkg->SubFunc);

	switch(mFunc)
	{
		case 1:
		{
			switch(sFunc)
			{
				case 1:
					   ClientHostName=pPkg->Msg;
					   Memo1->Lines->Add("New name"+ClientHostName);
					   Form1->Caption=ClientHostName;
					   Button1->Visible=true;
						break;
				case 2:
					   if(blnServer)
							Memo1->Lines->Add("Client"+pPkg->Host+"requests to disconnect.");
					   else
							Memo1->Lines->Add("Server"+pPkg->Host+"disconnected.");
					   break;
				case 3:
					  {
						str=pPkg->Msg;
						Memo1->Lines->Add(str);
						list->Delimiter = ',';
						list->DelimitedText =str;
						for(int i=0;i<25;i++)
						{
							nbnb[i]=list->Strings[i];
							num2[i]=StrToInt(nbnb[i]);
						}
						list->Clear();
						if(Button1->Visible==true)
						{
						set();
						Button1->Visible=false;
						}
						break;
					  }
			}
			break;
		}
		case 2:
		{
			click=true;
			for(int i=0;i<25;i++)
			{
				if(num2[i] == pPkg->Msg)
				{
					but[(i/5)][(i%5)]->Caption=pPkg->Msg;
					but[(i/5)][(i%5)]->Enabled = false;
				}
			}
			if(blnServer)
			{
				if(click==false)
					Label1->Caption="換他按";
				else
					Label1->Caption="換你按";
			}
			else
			{
				if(click==false)
					Label1->Caption="換他按";
				else
					Label1->Caption="換你按";
			}

			switch(sFunc)
			{

			  case 0:
			  {
				Memo1->Lines->Add("["+pPkg->Host+"]:"+pPkg->Msg);
                best();
				wwin();
				break;
			  }
			  case 1:
			  {
				player=1;
				Memo1->Lines->Add(player);
                best();
			  wwin();
				break;
			  }
			  case 2:
			  {
				player=2;
				Memo1->Lines->Add(player);
                best();
				wwin();
				break;
			  }
			  case 3:
			  {
				player=3;
				Memo1->Lines->Add(player);
                best();
				wwin();
					if(line>=3 &&sFunc>=3)
					{
						ShowMessage("Tie");
						del();
						Button1->Visible=true;
					}
					else if (line<3 &&sFunc>=3)
					{
						ShowMessage("lose");
                        del();
						Button1->Visible=true;
					}
					else if (line>=3 && sFunc<3)
					{
						ShowMessage("win");
                        del();
						Button1->Visible=true;
					}
			  }
				break;
			}
		}
			break;
		case 3:
		{
					Timer3->Enabled=false;
					if(line>=3 &&sFunc>=3)
					{
						ShowMessage("Tie");
                        del();
						Button1->Visible=true;
					}
					else if (line<3 &&sFunc>=3)
					{
						ShowMessage("lose");
                        del();
						Button1->Visible=true;
					}
					else if (line>=3 && sFunc<3)
					{
						ShowMessage("win");
                        del();
						Button1->Visible=true;
					}
					break;
		}
		default:
			break;
	}
}
void TForm1::ReflashClientList()
{

	for(int i=0;i<ServerSocket1->Socket->ActiveConnections;i++)
		Memo2->Lines->Add(ServerSocket1->Socket->Connections[i]->RemoteAddress+"-"+
								IntToStr(ServerSocket1->Socket->Connections[i]->SocketHandle));

}
void __fastcall TForm1::Timer2Timer(TObject *Sender)
{
	Timer2->Enabled=false;
	ReflashClientList();
}
void __fastcall TForm1::ServerSocket1Accept(TObject *Sender, TCustomWinSocket *Socket)
{
	fgConnectState=true;
	Memo2->Visible=true;
	ReflashClientList();
}
void __fastcall TForm1::ServerSocket1ClientDisconnect(TObject *Sender, TCustomWinSocket *Socket)
{
	Memo1->Lines->Add("'"+Socket->RemoteAddress+"-"+IntToStr(Socket->SocketHandle)+"'disconnect");
	StatusBar1->SimpleText="Status:Listening...";

	ShowMessage("斷線了");
	del();
	Button1->Visible=true;


	if(ServerSocket1->Socket->ActiveConnections==1)
	{
		fgConnectState=false;
	}
	Timer2->Enabled=true;

}
void __fastcall TForm1::ServerSocket1ClientError(TObject *Sender, TCustomWinSocket *Socket,
		  TErrorEvent ErrorEvent, int &ErrorCode)
{
	StatusBar1->SimpleText="Error from Client scoket.";
	ReflashClientList();
}
void __fastcall TForm1::ServerSocket1ClientRead(TObject *Sender, TCustomWinSocket *Socket)
{
	pPACKET pPkg;
	String strPacket;

	strPacket=Socket->ReceiveText();
	Memo1->Lines->Add(strPacket);
	pPkg=DisassemblePacket(strPacket);

	if(pPkg==NULL)
	{
		Memo2->Lines->Add("Packet error");
		return;
	}

	ParsePacket(pPkg);

}
void __fastcall TForm1::ServerSocket1ClientConnect(TObject *Sender, TCustomWinSocket *Socket)
{
	PACKET pkg;
	String strPkg;
	int clientNum;

	clientNum= ServerSocket1->Socket->ActiveConnections;

	if(clientNum>CLIENT_MAX)
	{
		Memo1->Lines->Add("已達連線最大數量");
		Socket->Close();
		return;
	}
	StatusBar1->SimpleText="Status:Connect from"+Socket->RemoteHost+"socket ID:"+IntToStr(Socket->SocketHandle);

	if(!CheckClients(Socket))
	{
	 pkg.ID="PON";
	 pkg.Host=ServerName;
	 pkg.separation="@";
	 pkg.MainFunc=1;
	 pkg.SubFunc=1;
	 pkg.MsgLength=newClient.Length();
	 pkg.Msg=newClient;
	 pkg.EndSymbol="#";
	 strPkg=AssemblePacket(pkg);
    }
	 ServerSocket1->Socket->Connections[clientNum-1]->SendText(strPkg);

}
bool TForm1::CheckClients(TCustomWinSocket *Socket)
{
	bool fg;
	int sHnd;
	String hostIP;

	hostIP=Socket->RemoteAddress;
	sHnd=Socket->SocketHandle;

	newClient=hostIP+"-"+IntToStr(sHnd);
	Memo1->Lines->Add("New connection"+newClient);
	Memo1->Lines->Add("Connection number:"+
							IntToStr(ServerSocket1->Socket->ActiveConnections));
    Button1->Visible=true;
	fg=false;
	return fg;
}
void TForm1::set()
{
	 int y = 0;
	 String AAA;
			for (int i = 0; i < 25; i++)
			{
				num[i]=(rand() % 25) +1;
				for (int j = 0; j < i; j++)
				{
					while (num[i] == num[j])
					{
						num[i] =(rand() % 25) +1;
						j = 0;
					}
				}
			}
			for (int i = 0; i < 5; i++)
			{
				for (int j = 0; j < 5; j++)
				{
					buttons[i][j] = new TButton(this);
					buttons[i][j]->Height=50;
					buttons[i][j]->Width=50;
					buttons[i][j]->Caption=IntToStr(num[y]);
					str+=IntToStr(num[y])+",";
					buttons[i][j]->Tag = num[y];
					buttons[i][j]->Left=80+j*50;
					buttons[i][j]->Top=80+i*50;
					buttons[i][j]->OnClick=cl;
					buttons[i][j]->Parent=this;
					y++;
				}
			}
			 Memo2->Lines->Add(str);
			for (int i = 0; i < 5; i++)
			{
				for (int j = 0; j < 5; j++)
				{
					but[i][j] = new TButton(this);
					but[i][j]->Height=50;
					but[i][j]->Width=50;
					but[i][j]->Caption="?";
					but[i][j]->Left=400+j*50;
					but[i][j]->Top=80+i*50;
					but[i][j]->Parent=this;
				}
			}
            String msg;
				PACKET pkg;
				String strPkg;
				String srtClient;

				pkg.ID="PON";

				if(blnServer)
					pkg.Host=IdIPWatch1->LocalIP();
				else
					pkg.Host=ClientHostName;

				pkg.separation="@";
				pkg.MainFunc=1;
				pkg.SubFunc=3;
				pkg.MsgLength=str.Length();
				pkg.Msg=str;
				pkg.EndSymbol="#";
				strPkg=AssemblePacket(pkg);
				Memo2->Lines->Add(strPkg);
				if (blnServer)
				{
					int index=0;

					srtClient=ServerSocket1->Socket->Connections[index]->RemoteAddress +
							 "-"+IntToStr(ServerSocket1->Socket->Connections[index]->SocketHandle);
					ServerSocket1->Socket->Connections[index]->SendText(strPkg);
				}
			else
				ClientSocket1->Socket->SendText(strPkg) ;
}
void __fastcall TForm1::cl(TObject *Sender)
{
	if(click)
	{
	String name;
	TButton *btn=(TButton*)Sender;
	btn->Enabled = false;
	Memo2->Lines->Add(btn->Tag);
	name = btn->Tag;

	for(int i=0;i<25;i++)
	 {
		 if(num2[i]==name)
		 {
			 but[(i/5)][(i%5)]->Caption=name;
			 but[(i/5)][(i%5)]->Enabled = false;
		 }
	 }
	 for (int i = 0; i < 5; i++)
			{
				for (int j = 0; j < 5; j++)
				{
					if (buttons[i][j]->Caption== name)
					{
						click1[i][j] = 1;
					}
				}
			}

	best();
		String msg;
		PACKET pkg;
		String strPkg;
		String srtClient;

		pkg.ID="PON";

	   if(blnServer)
		 pkg.Host=IdIPWatch1->LocalIP();
	   else
		 pkg.Host=ClientHostName;

	   pkg.separation="@";
	   pkg.MainFunc=2;
	   pkg.SubFunc=line;
	   pkg.MsgLength=IntToStr(btn->Tag).Length();
	   pkg.Msg=(btn->Tag);
	   pkg.EndSymbol="#";
	   strPkg=AssemblePacket(pkg);


	   if (blnServer)
		  {
			int index=0;
					srtClient=ServerSocket1->Socket->Connections[index]->RemoteAddress +
							 "-"+IntToStr(ServerSocket1->Socket->Connections[index]->SocketHandle);
				ServerSocket1->Socket->Connections[index]->SendText(strPkg);
		  }
		else
			ClientSocket1->Socket->SendText(strPkg) ;
		Timer3->Enabled=true;
        BB=0;
	   click=false;
	}
	if(blnServer)
	{
	  if(click==false)
		Label1->Caption="換他按";
	  else
		Label1->Caption="換你按";
	}
	else
	{
	if(click==false)
	  Label1->Caption="換他按";
	else
	  Label1->Caption="換你按";

	}
}
void TForm1::best()
{           line=0;
			//--------------------------------直----------------------------------------
			for (int i = 0; i < 5; i++)
			{
				if (click1[0][i] + click1[1][i] + click1[2][i] + click1[3][i] + click1[4][i] == 5)
				{
					line += 1;
				}
			}
			//--------------------------------橫-----------------------------------------
			for (int i = 0; i < 5; i++)
			{
				if (click1[i][0] + click1[i][1] + click1[i][2] + click1[i][3] + click1[i][4] == 5)
				{
					line += 1;
				}
			}

			//---------------------------------斜----------------------------------------
			if (click1[0][0] + click1[1][1]+ click1[2][2]+ click1[3][3]+ click1[4][4]== 5)
			{
				line += 1;
			}
			if (click1[0][4] + click1[1][3]+ click1[2][2]+ click1[3][1] + click1[4][0]== 5)
			{
				line += 1;
			}
			Memo2->Lines->Add("線"+IntToStr(line));
}
void TForm1::lineadd()
 {
   String msg;
				PACKET pkg;
				String strPkg;
				String srtClient;
				String lose="lose";

				pkg.ID="PON";

				if(blnServer)
					pkg.Host=IdIPWatch1->LocalIP();
				else
					pkg.Host=ClientHostName;

				pkg.separation="@";
				pkg.MainFunc=2;
				pkg.SubFunc=line;
				pkg.MsgLength=0;
				pkg.Msg="";
				pkg.EndSymbol="#";
				strPkg=AssemblePacket(pkg);

				if (blnServer)
				{
					int index=0;
					srtClient=ServerSocket1->Socket->Connections[index]->RemoteAddress +
							 "-"+IntToStr(ServerSocket1->Socket->Connections[index]->SocketHandle);
					ServerSocket1->Socket->Connections[index]->SendText(strPkg);

				}
			else
			{
				ClientSocket1->Socket->SendText(strPkg) ;
			}
 }
 void TForm1::wwin()
{
   String msg;
				PACKET pkg;
				String strPkg;
				String srtClient;

				pkg.ID="PON";

				if(blnServer)
					pkg.Host=IdIPWatch1->LocalIP();
				else
					pkg.Host=ClientHostName;

				pkg.separation="@";
				pkg.MainFunc=3;
				pkg.SubFunc=line;
				pkg.MsgLength=0;
				pkg.Msg="";
				pkg.EndSymbol="#";
				strPkg=AssemblePacket(pkg);

				if (blnServer)
				{
					int index=0;
					srtClient=ServerSocket1->Socket->Connections[index]->RemoteAddress +
							 "-"+IntToStr(ServerSocket1->Socket->Connections[index]->SocketHandle);
					ServerSocket1->Socket->Connections[index]->SendText(strPkg);

				}
			else
				ClientSocket1->Socket->SendText(strPkg) ;
 }
void __fastcall TForm1::Button1Click(TObject *Sender)
{
	set();
	Button1->Visible=false;
}
void TForm1::del()
{ int b=0;
  if( buttons[0][0]!=NULL)
  {
   for(int i=0;i<5;i++)
		{
			for(int j=0;j<5;j++)
			{
				num[b]=0;
				num2[b]=0;
				click1[i][j]=0;
				nbnb[b]="";
				b++;
			}
		}
    for(int i=0;i<5;i++)
	{
		for(int j=0;j<5;j++)
		{
			if(buttons[i][j]!=NULL)
				delete buttons[i][j];
			if(but[i][j]!=NULL)
				delete but[i][j];
		}
	}
	Label1->Caption="";
	click=true;
	str="";
	line = 0;
	player=0;
	Memo1->Clear();
	Memo2->Clear();
  }
}
void __fastcall TForm1::Timer3Timer(TObject *Sender)
{
	BB++;
	if(BB>5)
	{
        Timer3->Enabled=false;
		ShowMessage("連線逾時");
		if(blnServer)
			ServerSocket1->Active=false;
		else
            ClientSocket1->Active=false;
	}

        return;
}
//---------------------------------------------------------------------------

