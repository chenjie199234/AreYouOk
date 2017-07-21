#pragma once
//#include<wx/wxprec.h>
#include<wx/wx.h>
#include<wx/sckaddr.h>
#include<wx/socket.h>
#include<wx/protocol/http.h>
#include<wx/sstream.h>
#include<string.h>
enum
{
	Join_Or_Copy_BTN = 100,
	Setting_BTN,
	Exchange_BTN,
	Login_Or_Create_BTN,
	Reg_Or_Logout_BTN,
	Input_ID,
	Socket_ID,
	Dialog_ID,
	Dialog_Copy,
	Timer_ID,
};
class My_First_Project_Frame :public wxFrame
{
public:
	My_First_Project_Frame(const wxString &title);
	void OnJoinOrCopyClicked(wxCommandEvent& event);			//���뷿����Ʒ���Ű�ť�ص�
	void OnSettingClicked(wxCommandEvent& event);				//setting��ť�ص�
	void OnExchangeClicked(wxCommandEvent& event);				//״̬�л���ť�ص�
	void OnSocketEvent(wxSocketEvent& event);					//socket�����ص�
	void OnHotKeyPressed(wxKeyEvent& event);					//hotkey�ص�
	void OnLoginOrCreateClicked(wxCommandEvent& event);			//��½�򴴽���ť�ص�
	void OnRegOrLogoutClicked(wxCommandEvent& event);			//ע���ǳ���ť�ص�
	void OnInputSkip(wxKeyEvent& event);						//�û����������������������
	void OnTime(wxTimerEvent& event);							//��ʱ���ص�
	void OnQuit(wxCommandEvent& event);							//�˳��ص�

	int GetHotKey() { return hot_key; }
	void SetHotKey(int a) { hot_key = a; }
	int GetModKey() { return mod_key; }
	void SetModKey(int a) { mod_key = a; }
private:
	wxTimer * timer;							//��ʱ��
	wxPanel * panel;							//�հ����
	wxStaticText *num_unready;					//δ׼��
	wxStaticText *num_ready;					//׼��
	wxTextCtrl *input;							//�����
	wxTextCtrl *input_username;					//�û��������
	wxTextCtrl *input_passwd;					//���������
	wxButton *button_login_or_create;			//��½���ߴ������䰴ť
	wxButton *button_reg_or_logout;				//ע������˳���ť
	bool login_flag;							//�Ƿ��½״̬
	wxButton *button_join_or_copy;				//���뷿����߸��Ʒ����
	wxButton *setting;							//���ð�ť
	wxBitmapButton *exchange;					//״̬�л���ť
	wxSocketClient *s_client;					//���ӷ�������socket
	wxIPV4address addr;							//��������ַ
	bool join_room_tag;							//�Ƿ��Ѿ����뷿����
	bool exchange_tag;							//��ǰ״̬��־
	int hot_key;								//��ǰ��ͨ��ݼ�
	int mod_key;								//��ǰ�����ݼ�

	wxString version;							//�汾
};

class My_First_Project :public wxApp
{
public:
	bool OnInit()
	{
		wxString title = _T("Are You OK");
		frame = new My_First_Project_Frame(title);
		frame->SetMinSize(wxSize(380,185));
		frame->Show();
		frame->Center();
		return true;
	}
private:
	My_First_Project_Frame*frame;
};

class SetDialog :public wxDialog
{
public:
	SetDialog(My_First_Project_Frame*,const wxString&);
private:
	My_First_Project_Frame* parent;
};

IMPLEMENT_APP(My_First_Project);

My_First_Project_Frame::My_First_Project_Frame(const wxString &title) :wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(381, 186),wxDEFAULT_FRAME_STYLE|wxSTAY_ON_TOP),login_flag(false),join_room_tag(false),exchange_tag(true),hot_key(32),mod_key(0)
{
	//ͨ��http��÷�����ip
	/*
	wxHTTP http;
	http.SetMethod(_T("POST"));
	http.SetHeader(_T("Content-type"), _T("application/x-www-form-urlencoded"));
	while (!http.Connect(_T("www.areyouok.club")))
	{
		wxMessageBox(_T("Can't Connect To Http Server!\nPlease Check Your Net!\nWill Reconnect In 5 Seconds!"));
		wxSleep(5);
	}
	wxApp::IsMainLoopRunning();
	wxInputStream *httpstream = http.GetInputStream(_T("/IPAddr.txt"));
	if (http.GetError() == NOERROR)
	{
		wxString res;
		wxStringOutputStream out_stream(&res);
		httpstream->Read(out_stream);
		wxMessageBox(res);
		bool flag = false;
		wxString host;
		wxString port;
		for (unsigned int i = 0; i < wxStrlen(res); i++)
		{
			if (res[i] == ' ')
			{
				flag = true;
				continue;
			}
			switch (flag)
			{
			case true:port += res[i]; break;
			case false:host += res[i]; break;
			}
		}
		addr.Hostname(host);
		addr.Service(port);
	}
	else
	{
		wxMessageBox(_T("Can't Connect To Http Server!\nError Happened!"));
		Destroy();
	}
	wxDELETE(httpstream);
	http.Close();
	*/
	//д���汾
	version = "1.0.0";

	Connect(wxEVT_CLOSE_WINDOW, wxCommandEventHandler(My_First_Project_Frame::OnQuit));
	Connect(wxEVT_DESTROY, wxCommandEventHandler(My_First_Project_Frame::OnQuit));
	Connect(wxEVT_END_SESSION, wxCommandEventHandler(My_First_Project_Frame::OnQuit));
	//ע���ݼ�
	if (!RegisterHotKey(hot_key, mod_key, hot_key))
	{
		hot_key = 0;
		mod_key = 0;
		wxMessageBox(_T("Set New Hot Key Failed!\nConflict Happened!"));
	}
	//�����ö˿ں͵�ַ
	addr.Hostname(_T("areyouok.online"));
	//test addr
//	addr.Hostname(_T("192.168.18.121"));
	addr.Service(38146);
	//����socket
	s_client = new wxSocketClient();
	s_client->SetFlags(wxSOCKET_NOWAIT);
	s_client->SetEventHandler(*this, Socket_ID);
	Connect(Socket_ID, wxEVT_SOCKET, wxSocketEventHandler(My_First_Project_Frame::OnSocketEvent));
	s_client->SetNotify(/*wxSOCKET_CONNECTION_FLAG | */wxSOCKET_INPUT_FLAG| wxSOCKET_LOST_FLAG);
	s_client->Notify(true);

	//������ʱ��
	timer = new wxTimer(this, Timer_ID);
	Connect(Timer_ID, wxEVT_TIMER, wxTimerEventHandler(My_First_Project_Frame::OnTime));
	timer->Start(3000);
	//����һ������panel
	panel = new wxPanel(this, wxID_ANY);
	//����һ���ܵ� ���ֿؼ�
	wxBoxSizer * total_box = new wxBoxSizer(wxVERTICAL);
	//-----------------------------��һ��--------------------------------
	wxStaticText * st_username = new wxStaticText(panel, wxID_ANY, _T("Username:"));
	wxStaticText * st_passwd = new wxStaticText(panel, wxID_ANY, _T("Passwd:"));
	input_username = new wxTextCtrl(panel, wxID_ANY, _T(""),wxDefaultPosition,wxSize(60,-1));
	input_username->Connect(wxEVT_CHAR, wxKeyEventHandler(My_First_Project_Frame::OnInputSkip));
	input_passwd = new wxTextCtrl(panel, wxID_ANY, _T(""),wxDefaultPosition,wxSize(60,-1));
	input_passwd->Connect(wxEVT_CHAR, wxKeyEventHandler(My_First_Project_Frame::OnInputSkip));
	button_login_or_create = new wxButton(panel, Login_Or_Create_BTN, _T("Login"), wxDefaultPosition, wxSize(60, -1));
	Connect(Login_Or_Create_BTN, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(My_First_Project_Frame::OnLoginOrCreateClicked));
	button_reg_or_logout = new wxButton(panel, Reg_Or_Logout_BTN, _T("Register"), wxDefaultPosition, wxSize(60, -1));
	Connect(Reg_Or_Logout_BTN, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(My_First_Project_Frame::OnRegOrLogoutClicked));
	wxBoxSizer* First_line_box = new wxBoxSizer(wxHORIZONTAL);
	First_line_box->Add(st_username,0,wxCENTER);
	First_line_box->Add(input_username, 1,wxCENTER);
	First_line_box->Add(st_passwd,0,wxCENTER);
	First_line_box->Add(input_passwd, 1,wxCENTER);
	First_line_box->Add(button_login_or_create,0);
	First_line_box->Add(button_reg_or_logout,0);
	total_box->Add(First_line_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5);
	//-----------------------------�ڶ���--------------------------------
	//����һ�� �����
	input = new wxTextCtrl(panel, Input_ID, _T(""));
	//����һ�� ȷ�ϰ�ť
	button_join_or_copy = new wxButton(panel, Join_Or_Copy_BTN, _T("Join"));
	//����ȷ�ϰ�ť�Ķ���
	Connect(Join_Or_Copy_BTN, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(My_First_Project_Frame::OnJoinOrCopyClicked));
	//����һ�� ���ð�ť
	setting = new wxButton(panel, Setting_BTN, _("Setting"));
	//�������ð�ť�Ķ���
	Connect(Setting_BTN, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(My_First_Project_Frame::OnSettingClicked));
	//hotkey�ص�
	Connect(wxEVT_HOTKEY, wxKeyEventHandler(My_First_Project_Frame::OnHotKeyPressed));
	//Ϊ ������ȷ�ϰ�ť ����һ��ˮƽ ���ֿؼ�
	wxBoxSizer * input_box = new wxBoxSizer(wxHORIZONTAL);
	input_box->Add(setting, 0,wxRIGHT,3);
	input_box->Add(input, 1,wxTOP,3);
	input_box->Add(button_join_or_copy, 0, wxLEFT, 5);
	total_box->Add(input_box, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);
	//-----------------------------������-------------------------------------
	//���ڴ��׼���õ��˵� ͼƬ��title���Ͷ�̬�ı������
	//��� �ܵ�
	wxBoxSizer * left_total_box = new wxBoxSizer(wxHORIZONTAL);
	//��� ͼƬ
	//��� ��Ϣ
	wxBoxSizer * left_img_box = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer * left_message_box = new wxBoxSizer(wxVERTICAL);
	//�ұ� �ܵ�
	wxBoxSizer * right_total_box = new wxBoxSizer(wxHORIZONTAL);
	//�ұ� ͼƬ
	//�ұ� ��Ϣ
	wxBoxSizer * right_img_box = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer * right_message_box = new wxBoxSizer(wxVERTICAL);
	//�м�
	wxBoxSizer * mid_box = new wxBoxSizer(wxHORIZONTAL);
	//�ܵ�
	wxBoxSizer * message_box = new wxBoxSizer(wxHORIZONTAL);
	//׼���ã�û��ת���õ��ַ���
	wxStaticText *on_fire = new wxStaticText(panel, wxID_ANY, _T("OK! OK!"));
	wxStaticText *on_sleep = new wxStaticText(panel, wxID_ANY, _T("NOT OK!"));
	//����������ʾ״̬������
	num_unready = new wxStaticText(panel, wxID_ANY, _T("0"));
	num_ready = new wxStaticText(panel, wxID_ANY, _T("0"));
	//ͼƬ
	wxImage::AddHandler(new wxPNGHandler);
	wxBitmap on_fire_png("img\\fire.png", wxBITMAP_TYPE_PNG);
	wxBitmap on_sleep_png("img\\sleep.png", wxBITMAP_TYPE_PNG);
	wxBitmap exchange_png("img\\exchange.png", wxBITMAP_TYPE_PNG);
	//���
	wxStaticBitmap* static_icon_fire = new wxStaticBitmap(panel, wxID_ANY, on_fire_png);
	left_img_box->Add(static_icon_fire);
	left_message_box->Add(on_fire);
	left_message_box->Add(num_ready);
	left_total_box->Add(left_img_box);
	left_total_box->Add(left_message_box,0,wxALIGN_CENTER);
	//�ұ�
	wxStaticBitmap* static_icon_sleep = new wxStaticBitmap(panel, wxID_ANY, on_sleep_png);
	right_img_box->Add(static_icon_sleep);
	right_message_box->Add(on_sleep,0,wxALIGN_RIGHT);
	right_message_box->Add(num_unready,0);
	right_total_box->Add(right_message_box,0,wxALIGN_CENTER);
	right_total_box->Add(right_img_box,0);
	//�м�
	exchange =new wxBitmapButton(panel, Exchange_BTN, exchange_png);
	Connect(Exchange_BTN, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(My_First_Project_Frame::OnExchangeClicked));
	exchange->SetBackgroundColour(wxColour(0, 255, 0));
	mid_box->Add(exchange,0);
	//���
	wxBoxSizer *fill_box_1 = new wxBoxSizer(wxHORIZONTAL);
	fill_box_1->Add(-1, 1);
	wxBoxSizer *fill_box_2 = new wxBoxSizer(wxHORIZONTAL);
	fill_box_2->Add(-1, 1);
	message_box->Add(left_total_box,0);
	message_box->Add(fill_box_1, 1);
	message_box->Add(mid_box,0);
	message_box->Add(fill_box_2, 1);
	message_box->Add(right_total_box,0);

	total_box->Add(message_box,0,wxEXPAND|wxLEFT|wxRIGHT,5);

	//���ܵĲ��ֿؼ� ���ӵ�panel��
	panel->SetSizer(total_box);
	//-----------------------------������-----------------------------
	wxMessageBox(_T("Welcome!This Is A Completely Free App!\nTCP Packets Wasn't Encoded,Don't Use Your familiar Acount To Reg!\nIf This App Is Useful To You,And You Want To Support It!\nYou Can Donate For It On The WebSite!\nThe Money Will Be Used To Maintain The Server And Domain!\nHave Fun,Thanks!\nWebSite:www.areyouok.online"));
}
void My_First_Project_Frame::OnQuit(wxCommandEvent & event)
{
	timer->Stop();
	if(s_client->IsConnected())
		s_client->Close();
	Destroy();
}
void My_First_Project_Frame::OnTime(wxTimerEvent& event)
{
	if (s_client->IsConnected())
	{
		char head = '6';
		s_client->Write(&head, 1);
	}
}
void My_First_Project_Frame::OnInputSkip(wxKeyEvent& event)
{
	//ȥ��������ţ�ֻ�д�СдӢ�ĺ�����
	if (((event.GetKeyCode() >=48  && event.GetKeyCode() <=57) || (event.GetKeyCode()>=65 && event.GetKeyCode()<=90) || (event.GetKeyCode()>=97 && event.GetKeyCode()<=122) || event.GetKeyCode() == 8) && event.GetKeyCode()!=47 && event.GetKeyCode()!=32)
		event.Skip();
//	if (event.GetKeyCode())
//		wxMessageBox(wxString::Format(_T("%d"), event.GetKeyCode()));
}
void My_First_Project_Frame::OnLoginOrCreateClicked(wxCommandEvent& event)
{
	if (login_flag)
	{
		//�ѵ�½
		if (s_client->IsConnected())
		{
			//���ʹ���������Ϣ
			char head = '5';
			button_login_or_create->Enable(false);
			input->Enable(false);
			button_join_or_copy->Enable(false);
			s_client->Write(&head, 1);
		}
		else
		{
			wxMessageBox(_T("Login But Not Connected!\nError Happened!\nDestroy!"));
			Destroy();
		}
	}
	else
	{
		//δ��½
		if (s_client->IsConnected())
		{
			//���͵�½��Ϣ
			char head = '3';
			wxString username_str = input_username->GetLineText(1);
			wxString passwd_str = input_passwd->GetLineText(1);
			int len = username_str.Len() + passwd_str.Len();
			if (username_str.Len() !=0 && passwd_str.Len() != 0 && len < 250)
			{
				unsigned char total_length = len + 1;
				wxString total_str = head;
				total_str += total_length;
				total_str += username_str;
				total_str += _T("$");
				total_str += passwd_str;
				input_username->Enable(false);
				input_passwd->Enable(false);
				button_login_or_create->Enable(false);
				button_reg_or_logout->Enable(false);
				s_client->Write(total_str.mb_str(), wxStrlen(total_str));
			}
			else
				wxMessageBox(_T("Username And Passwd Too Long!"));
		}
		else
		{
			if (!s_client->Connect(addr, true))
			{
				wxMessageBox(_T("Can't Connected To TCP Server!"));
			}
			else
			{
				//���͵�½��Ϣ
				char head = '3';
				wxString username_str = input_username->GetLineText(1);
				wxString passwd_str = input_passwd->GetLineText(1);
				int len = username_str.Len() + passwd_str.Len();
				if (username_str.Len() != 0 && passwd_str.Len() != 0 && len < 250)
				{
					unsigned char total_length = len + 1;
					wxString total_str = head;
					total_str += total_length;
					total_str += username_str;
					total_str += _T("$");
					total_str += passwd_str;
					input_username->Enable(false);
					input_passwd->Enable(false);
					button_login_or_create->Enable(false);
					button_reg_or_logout->Enable(false);
					s_client->Write(total_str.mb_str(), wxStrlen(total_str));;
				}
				else
					wxMessageBox(_T("Username And Passwd Too Long Or Empty!"));
			}
		}
	}
}
void My_First_Project_Frame::OnRegOrLogoutClicked(wxCommandEvent& event)
{
	if (login_flag)
	{
		//�ѵ�½
		if (s_client->IsConnected())
		{
			//���͵ǳ���Ϣ
			s_client->Close();
			login_flag = false;
			//����
			input->Enable(true);
			input_username->Enable(true);
			input_passwd->Enable(true);

			join_room_tag = false;
			button_join_or_copy->SetLabel(_T("Join"));
			button_join_or_copy->Enable(true);

			button_login_or_create->Enable(true);
			button_reg_or_logout->Enable(true);
			button_login_or_create->SetLabel(_T("Login"));
			button_reg_or_logout->SetLabel(_T("Register"));

			exchange->Enable(true);
			exchange_tag = true;
			exchange->SetBackgroundColour(wxColour(0, 255, 0));

			num_ready->SetLabel(_T("0"));
			num_unready->SetLabel(_T("0"));
			wxMessageBox(_T("Close Connection Success!"));
		}
		else
		{
			wxMessageBox(_T("Login But Not Connected!\nError Happened!\nDestroy!"));
			Destroy();
		}
	}
	else
	{
		//δ��½
		if (s_client->IsConnected())
		{
			//����ע����Ϣ
			char head = '4';
			wxString username_str = input_username->GetLineText(1);
			wxString passwd_str = input_passwd->GetLineText(1);
			int len = username_str.Len() + passwd_str.Len();
			if (username_str.Len() != 0 && passwd_str.Len() != 0 && len < 250)
			{
				unsigned char total_length = len + 1;
				wxString total_str = head;
				total_str += total_length;
				total_str += username_str;
				total_str += _T("$");
				total_str += passwd_str;
				input_username->Enable(false);
				input_passwd->Enable(false);
				button_login_or_create->Enable(false);
				button_reg_or_logout->Enable(false);
				s_client->Write(total_str.mb_str(), wxStrlen(total_str));
			}
			else
				wxMessageBox(_T("Username And Passwd Too Long Or Empty!"));
		}
		else
		{
			if (!s_client->Connect(addr, true))
			{
				wxMessageBox(_T("Can't Connected To TCP Server!"));
			}
			else
			{
				//����ע����Ϣ
				char head = '4';
				wxString username_str = input_username->GetLineText(1);
				wxString passwd_str = input_passwd->GetLineText(1);
				int len = username_str.Len() + passwd_str.Len();
				if (username_str.Len() != 0 && passwd_str.Len() != 0 && len < 250)
				{
					unsigned char total_length = len + 1;
					wxString total_str = head;
					total_str += total_length;
					total_str += username_str;
					total_str += _T("$");
					total_str += passwd_str;
					input_username->Enable(false);
					input_passwd->Enable(false);
					button_login_or_create->Enable(false);
					button_reg_or_logout->Enable(false);
					s_client->Write(total_str.mb_str(), wxStrlen(total_str));
				}
				else
					wxMessageBox(_T("Username And Passwd Too Long Or Empty!"));
			}
		}
	}
}
void My_First_Project_Frame::OnJoinOrCopyClicked(wxCommandEvent &event)
{
	//ÿ���������ӵ�ʱ������״̬
	if (join_room_tag)
	{
		input->SelectAll();
		input->Copy();
		wxMessageBox(_T("Copy Success!"));
	}
	else
	{
		exchange_tag = true;
		exchange->SetBackgroundColour(wxColour(0, 255, 0));
		if (s_client->IsConnected())
		{
			//���ͼ��뷿����Ϣ
			char head = '2';
			wxString unique_num = head;
			wxString room_str = input->GetLineText(1);
			unique_num += room_str;
			if (room_str.Len() != 16)
				wxMessageBox(_T("Wrong Room Number!"));
			else
			{
				input->Enable(false);
				button_join_or_copy->Enable(false);
				s_client->Write(unique_num.mb_str(), wxStrlen(unique_num));
			};
		}
		else
		{
			if (!s_client->Connect(addr, true))
			{
				wxMessageBox(_T("Can't Connected To TCP Server!"));
			}
			else
			{
				//���ͼ��뷿����Ϣ
				char head = '2';
				wxString unique_num = head;
				wxString room_str = input->GetLineText(1);
				unique_num += room_str;
				if (room_str.Len() != 16)
					wxMessageBox(_T("Wrong Room Number Length!"));
				else
				{
					input->Enable(false);
					button_join_or_copy->Enable(false);
					s_client->Write(unique_num.mb_str(), wxStrlen(unique_num));
				}
			}
		}
	}
}
void My_First_Project_Frame::OnSettingClicked(wxCommandEvent &event)
{
	SetDialog * dialog = new SetDialog(this,_T("Set Hot Key"));
	dialog->Show();
	dialog->Destroy();
}
void My_First_Project_Frame::OnHotKeyPressed(wxKeyEvent& event)
{
	const int keycode = event.GetKeyCode();
	const int modifiercode = event.GetModifiers();
	if (keycode == hot_key && modifiercode == mod_key)
	{
		if (s_client->IsConnected() && join_room_tag)
		{
			char a;
			if (exchange_tag)
			{
				a = '0';
				//��ֹ��ε����ֻ���յ���������Ϣ������ٵ�
				exchange->Enable(false);
			}
			else
			{
				a = '1';
				//��ֹ��ε����ֻ���յ���������Ϣ������ٵ�
				exchange->Enable(false);
			}
			s_client->Write(&a, 1);
		}
		else if (s_client->IsDisconnected())
			wxMessageBox(_T("You Are Disconnected To TCP Server!\nPlease Connected To Server First!"));
		else
			wxMessageBox(_T("You Are Not In A Room!l\nJoin A Room First!"));
	}
}
void My_First_Project_Frame::OnExchangeClicked(wxCommandEvent &event)
{
	//����������״̬ ���� ���뷿���˲��ܽ���״̬�л�
	if (s_client->IsConnected() && join_room_tag)
	{
		char a;
		if (exchange_tag)
		{
			a = '0';
			//��ֹ��ε����ֻ���յ���������Ϣ������ٵ�
			exchange->Enable(false);
		}
		else
		{
			a = '1';
			//��ֹ��ε����ֻ���յ���������Ϣ������ٵ�
			exchange->Enable(false);
		}
		s_client->Write(&a, 1);
	}
	else if (s_client->IsDisconnected())
		wxMessageBox(_T("You Are Disconnected To TCP Server!\nPlease Connected To Server First!"));
	else
		wxMessageBox(_T("You Are Not In A Room!l\nJoin A Room First!"));
}
void My_First_Project_Frame::OnSocketEvent(wxSocketEvent &event)
{
	switch (event.GetSocketEvent())
	{
	//�ɶ�
	case wxSOCKET_INPUT:
	{
		char status;
		while (1)
		{
			s_client->Peek(&status, 1);
			if (s_client->LastCount() == 0)
				break;
			switch (status)
			{
			case '0'://û��׼����
			{
				char tempbuffer[9];
				s_client->Peek(tempbuffer, 9);
				if (s_client->LastCount() == 9)
				{
					//������ճ��
					char head;
					char ready_buffer[4];
					char unreaddy_buffer[4];
					s_client->Read(&head, 1);
					s_client->Read(ready_buffer, 4);
					s_client->Read(unreaddy_buffer, 4);
					unsigned int ready = 0;
					unsigned int unready = 0;
					memcpy(&ready, ready_buffer, 4);
					memcpy(&unready, unreaddy_buffer, 4);
					ready = wxUINT32_SWAP_ON_LE(ready);
					unready = wxUINT32_SWAP_ON_LE(unready);
//					ready = ntohl(ready);
//					unready = ntohl(unready);
					num_ready->SetLabel(wxString::Format(_T("%d"), ready));
					num_unready->SetLabel(wxString::Format(_T("%d"), unready));
					exchange_tag = false;
					exchange->SetBackgroundColour(wxColour(255, 0, 0));
					exchange->Enable(true);
				}
				else
					//����ճ��
					break;
			}
			break;
			case '1'://׼������
			{
				char tempbuffer[9];
				s_client->Peek(tempbuffer, 9);
				if (s_client->LastCount() == 9)
				{
					//������ճ��
					char head;
					char ready_buffer[4];
					char unreaddy_buffer[4];
					s_client->Read(&head, 1);
					s_client->Read(ready_buffer, 4);
					s_client->Read(unreaddy_buffer, 4);
					unsigned int ready = 0;
					unsigned int unready = 0;
					memcpy(&ready, ready_buffer, 4);
					memcpy(&unready, unreaddy_buffer, 4);
					ready = wxUINT32_SWAP_ON_LE(ready);
					unready = wxUINT32_SWAP_ON_LE(unready);
//					ready = ntohl(ready);
//					unready = ntohl(unready);
					num_ready->SetLabel(wxString::Format(_T("%d"), ready));
					num_unready->SetLabel(wxString::Format(_T("%d"), unready));
					exchange_tag = true;
					exchange->SetBackgroundColour(wxColour(0, 255, 0));
					exchange->Enable(true);
				}
				else
					//����ճ��
					break;
			}
			break;
			case '2'://���뷿��ʧ��
			{
				char head;
				s_client->Read(&head, 1);
				input->Enable(true);
				button_join_or_copy->Enable(true);
				wxMessageBox(_T("Room Does Not Exist!"));
			}
			break;
			case '3'://���뷿��ɹ�
			{
				char tempbuffer[9];
				s_client->Peek(tempbuffer, 9);
				if (s_client->LastCount() == 9)
				{
					//������ճ��
					char head;
					char ready_buffer[4];
					char unreaddy_buffer[4];
					s_client->Read(&head, 1);
					s_client->Read(ready_buffer, 4);
					s_client->Read(unreaddy_buffer, 4);
					unsigned int ready = 0;
					unsigned int unready = 0;
					memcpy(&ready, ready_buffer, 4);
					memcpy(&unready, unreaddy_buffer, 4);
					ready = wxUINT32_SWAP_ON_LE(ready);
					unready = wxUINT32_SWAP_ON_LE(unready);
//					ready = ntohl(ready);
//					unready = ntohl(unready);
					num_ready->SetLabel(wxString::Format(_T("%d"), ready));
					num_unready->SetLabel(wxString::Format(_T("%d"), unready));
					join_room_tag = true;
					button_join_or_copy->SetLabel(_T("Copy"));
					button_join_or_copy->Enable(true);
					exchange_tag = true;
					exchange->SetBackgroundColour(wxColour(0, 255, 0));
					exchange->Enable(true);
					wxMessageBox(_T("Join Room Sucess!"));
				}
				else
					//����ճ��
					break;
			}
			break;
			case '4'://��½����ע��ɹ�
			{
				char head;
				s_client->Read(&head, 1);
				login_flag = true;
				button_login_or_create->Enable(true);
				button_reg_or_logout->Enable(true);
				button_login_or_create->SetLabel(_T("Create"));
				button_reg_or_logout->SetLabel(_T("Logout"));
				if (join_room_tag)
					button_login_or_create->Enable(false);
				wxMessageBox(_T("Welcome To Use AreYouOK!"));
			}
			break;
			case '5'://��½ʧ��
			{
				char head;
				s_client->Read(&head, 1);
				input_username->Enable(true);
				input_passwd->Enable(true);
				button_login_or_create->Enable(true);
				button_reg_or_logout->Enable(true);
				wxMessageBox(_T("Wrong Username Or Wrong Passwd!"));
			}
			break;
			case '6'://ע��ʧ��
			{
				char head;
				s_client->Read(&head, 1);
				input_username->Enable(true);
				input_passwd->Enable(true);
				button_login_or_create->Enable(true);
				button_reg_or_logout->Enable(true);
				wxMessageBox(_T("Username Exist!"));
			}
			break;
			case '7'://��������ɹ�
			{
				char tempbuffer[17];
				s_client->Peek(tempbuffer, 17);
				if (s_client->LastCount() == 17)
				{
					//������ճ��
					char head;
					char roombuffer[17];
					s_client->Read(&head, 1);
					s_client->Read(roombuffer, 16);
					roombuffer[16] = '\0';
					//�������д��input��
					input->SetValue(roombuffer);
					num_ready->SetLabel(_T("1"));
					num_unready->SetLabel(_T("0"));
					join_room_tag = true;
					button_join_or_copy->SetLabel(_T("Copy"));
					button_join_or_copy->Enable(true);
					exchange_tag = true;
					exchange->SetBackgroundColour(wxColour(0, 255, 0));
					exchange->Enable(true);
					wxMessageBox(_T("Create Room Sucess!\nYou Are Already In The Room!"));
				}
				else
					//����ճ��
					break;
			}
			break;
			case '8'://���ܷ�������֪ͨ��Ϣ
			{
				unsigned int msg_length = 0;
				char msg_head[5];
				s_client->Peek(msg_head, 5);
				if (s_client->LastCount() == 5)
				{
					memcpy(&msg_length, &msg_head[1], 4);
					msg_length = wxUINT32_SWAP_ON_LE(msg_length);
					char * msg = new char[msg_length + 5];
					s_client->Peek(msg, (msg_length + 5));
					if (s_client->LastCount() == (msg_length + 5))
					{
						//������ճ��
						s_client->Read(msg, (msg_length + 5));
						wxString str;
						for (unsigned int i = 0; i < msg_length; i++)
						{
							str += msg[5 + i];
						}
						wxMessageBox(str);
					}
					else
						//����ճ��
						break;
					delete []msg;
				}
				else
					//����ճ��
					break;
			}
			break;
			case '9'://��������
			{
				char tempbuffer[9];
				s_client->Peek(tempbuffer, 9);
				if (s_client->LastCount() == 9)
				{
					//������ճ��
					char head;
					char ready_buffer[4];
					char unreaddy_buffer[4];
					s_client->Read(&head, 1);
					s_client->Read(ready_buffer, 4);
					s_client->Read(unreaddy_buffer, 4);
					unsigned int ready = 0;
					unsigned int unready = 0;
					memcpy(&ready, ready_buffer, 4);
					memcpy(&unready, unreaddy_buffer, 4);
					ready = wxUINT32_SWAP_ON_LE(ready);
					unready = wxUINT32_SWAP_ON_LE(unready);
//					ready = ntohl(ready);
//					unready = ntohl(unready);
					num_ready->SetLabel(wxString::Format(_T("%d"), ready));
					num_unready->SetLabel(wxString::Format(_T("%d"), unready));
				}
			}
			break;
			case 'a'://�汾��֤
			{
				unsigned int msg_length = 0;
				char msg_head[5];
				s_client->Peek(msg_head, 5);
				if (s_client->LastCount() == 5)
				{
					memcpy(&msg_length, &msg_head[1], 4);
					msg_length = wxUINT32_SWAP_ON_LE(msg_length);
					char * msg = new char[msg_length + 5];
					s_client->Peek(msg, (msg_length + 5));
					if (s_client->LastCount() == (msg_length + 5))
					{
						//������ճ��
						s_client->Read(msg, (msg_length + 5));
						wxString str;
						for (unsigned int i = 0; i < msg_length; i++)
						{
							str += msg[5 + i];
						}
						if (str != version)
						{
							wxMessageBox(_T("Wrong Version!\nPlease Update The App!"));
							Close(true);
						}
					}
					else
						//����ճ��
						break;
					delete[]msg;
				}
				else
					//����ճ��
					break;
			}
			break;
			default:
			{
				wxMessageBox(_T("Message Type Error!"));
				Destroy();
			}
			break;
			}
		}
	}
	break;
	//�Ͽ�
	case wxSOCKET_LOST:
	{
		//�ر�socket
		s_client->Close();
		login_flag = false;
		//����
		input->Enable(true);
		input_username->Enable(true);
		input_passwd->Enable(true);

		join_room_tag = false;
		button_join_or_copy->SetLabel(_T("Join"));
		button_join_or_copy->Enable(true);

		button_login_or_create->Enable(true);
		button_reg_or_logout->Enable(true);
		button_login_or_create->SetLabel(_T("Login"));
		button_reg_or_logout->SetLabel(_T("Register"));

		exchange->Enable(true);
		exchange_tag = true;
		exchange->SetBackgroundColour(wxColour(0, 255, 0));

		num_ready->SetLabel(_T("0"));
		num_unready->SetLabel(_T("0"));
		wxMessageBox(_T("Lost Connection To TCP Server!"));
	}
	break;
	default:
		break;
	}
}

//�������
SetDialog::SetDialog(My_First_Project_Frame* frame, const wxString &title):wxDialog(frame,wxID_ANY,title,wxDefaultPosition,wxSize(300,140)),parent(frame)
{
	SetFocus();
	wxPanel * panel = new wxPanel(this, wxID_ANY);
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	int h_k = parent->GetHotKey();
	int m_k = parent->GetModKey();
	parent->SetHotKey(0);
	parent->SetModKey(0);
	if (h_k)
	{
		if (!parent->UnregisterHotKey(h_k))
			wxMessageBox(_T("Unregister Hot Key failed!"));
	}
	else
		wxMessageBox(_T("No Hot Key Redistered!"));
	wxString key_special;
	if (m_k & wxMOD_CONTROL)
		key_special = _T("Ctrl");
	else if (m_k & wxMOD_SHIFT)
		key_special = _T("Shift");
	else if (m_k & wxMOD_ALT)
		key_special = _T("Alt");
	wxString key_normal;
	if (h_k == 32)
		key_normal = _T("Space");
	else if (h_k == 48)
		key_normal = _T("0");
	else if (h_k == 49)
		key_normal = _T("1");
	else if (h_k == 50)
		key_normal = _T("2");
	else if (h_k == 51)
		key_normal = _T("3");
	else if (h_k == 52)
		key_normal = _T("4");
	else if (h_k == 53)
		key_normal = _T("5");
	else if (h_k == 54)
		key_normal = _T("6");
	else if (h_k == 55)
		key_normal = _T("7");
	else if (h_k == 56)
		key_normal = _T("8");
	else if (h_k == 57)
		key_normal = _T("9");
	//-------------------------��һ��------------------------------
	wxStaticText * message = new wxStaticText(panel, wxID_ANY, _T("Select New Key As Your New Hot Key!\nSupport Keys : (Ctrl / Shift / Alt) + (0-9) !"));
	//-------------------------�ڶ���------------------------------
	wxStaticText * now_key;
	if (wxStrlen(key_special))
		now_key = new wxStaticText(panel, wxID_ANY, wxString::Format(_T("Now Hot Key : %s - %s"), key_special, key_normal));
	else
		now_key = new wxStaticText(panel, wxID_ANY, wxString::Format(_T("Now Hot Key : %s"), key_normal));
	//-------------------------������------------------------------
	wxBoxSizer * hbox = new wxBoxSizer(wxHORIZONTAL);
	//�����ݼ�
	wxStaticText *choice_mod_text = new wxStaticText(panel, wxID_ANY, _T("Special:"));
	wxString mod_str[] = { _T("Don't Use"),_T("Ctrl"),_T("Shift"),_T("Alt") };
	wxChoice* choice_mod = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(80, -1), WXSIZEOF(mod_str), mod_str, 0);
	if (wxStrlen(key_special))
		choice_mod->SetStringSelection(key_special);
	else
		choice_mod->SetStringSelection(_T("Don't Use"));
	//������ݼ�
	wxStaticText *choice_hot_text = new wxStaticText(panel, wxID_ANY, _T("Normal:"));
	wxString hot_str[] = { _T("0"),_T("1"),_T("2"),_T("3"),_T("4"),_T("5"),_T("6"),_T("7"),_T("8"),_T("9"),_T("Space") };
	wxChoice* choice_hot = new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxSize(80, -1), WXSIZEOF(hot_str), hot_str, 0);
	choice_hot->SetStringSelection(wxString::Format(_T("%s"), key_normal));

	hbox->Add(choice_mod_text,0,wxALIGN_CENTER);
	hbox->Add(choice_mod,0,wxLEFT,5);
	hbox->Add(choice_hot_text,0,wxALIGN_CENTER|wxLEFT,5);
	hbox->Add(choice_hot,0,wxLEFT,5);
	vbox->Add(message,1,wxLEFT|wxTOP,5);
	vbox->Add(now_key,1,wxLEFT|wxTOP,5);
	vbox->Add(hbox,0,wxALL,5);
	panel->SetSizer(vbox);
	Center();
	ShowModal();
	wxString temp_mod = choice_mod->GetStringSelection();
	wxString temp_hot = choice_hot->GetStringSelection();
	if (temp_mod == _T("Ctrl"))
		m_k = wxMOD_CONTROL;
	else if (temp_mod == _T("Shift"))
		m_k = wxMOD_SHIFT;
	else if (temp_mod == _T("Alt"))
		m_k = wxMOD_ALT;
	else
		m_k = 0;
	if (temp_hot == _T("0"))
		h_k = 48;
	else if (temp_hot == _T("1"))
		h_k = 49;
	else if (temp_hot == _T("2"))
		h_k = 50;
	else if (temp_hot == _T("3"))
		h_k = 51;
	else if (temp_hot == _T("4"))
		h_k = 52;
	else if (temp_hot == _T("5"))
		h_k = 53;
	else if (temp_hot == _T("6"))
		h_k = 54;
	else if (temp_hot == _T("7"))
		h_k = 55;
	else if (temp_hot == _T("8"))
		h_k = 56;
	else if (temp_hot == _T("9"))
		h_k = 57;
	else if (temp_hot == _T("Space"))
		h_k = 32;
	parent->SetHotKey(h_k);
	parent->SetModKey(m_k);
	if (!parent->RegisterHotKey(h_k, m_k, h_k))
		wxMessageBox(_T("Set New Hot Key Failed!\nConflict Happened!"));
	Destroy();
}
