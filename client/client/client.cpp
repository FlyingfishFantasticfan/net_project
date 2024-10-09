#include <httplib.h>

#include <graphics.h>
#include <cJSON.h>


#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <sstream>

#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")

int player_id = 0;// ���ID

std::string str_address;			// ��������ַ
httplib::Client* client = nullptr;	// HTTP�ͻ��˶���
int plate[10] = { 0 };	// ����������


void load_resources(HWND hwnd);// ������Դ
void login_to_server(HWND hwnd);// ��¼��������
void draw_plate();// ��������

int main()
{
	HWND hwnd = initgraph(720, 720/*, EW_SHOWCONSOLE*/);
	SetWindowText(hwnd, _T("������"));


	load_resources(hwnd);
	login_to_server(hwnd);

	ExMessage msg;



	BeginBatchDraw();

	while (true)
	{

		///�����������///
		while (peekmessage(&msg))
		{
			if (msg.message == WM_LBUTTONDOWN)
			{
				int x = msg.x;
				int y = msg.y;
				// �������ĸ�������
				int row = y / 240;
				int col = x / 240;
				int index = row * 3 + col;
				// ������ĸ����Ƿ�Ϊ��
				if (plate[index] == 0)
				{
					int move_info = player_id * 10 + index;

					std::string route = "/move";
					std::string body = std::to_string(move_info);
					httplib::Result result = client->Post(route.c_str(), body, "text/plain");

				}
			}
		}


		///�������///
		if (plate[9] == 1)
		{
			MessageBox(hwnd, _T("���1��ʤ��"), _T("��Ϸ����"), MB_OK | MB_ICONINFORMATION);
			break;
		}
		else if (plate[9] == 2)
		{
			MessageBox(hwnd, _T("���2��ʤ��"), _T("��Ϸ����"), MB_OK | MB_ICONINFORMATION);
			break;
		}


		///����///
		setbkcolor(RGB(0, 0, 0));
		cleardevice();

		draw_plate();


		FlushBatchDraw();
	}

	EndBatchDraw();
	return 0;
}


void load_resources(HWND hwnd)// ������Դ
{
	std::ifstream file("config.cfg");
	if (!file.good())
	{
		MessageBox(hwnd, L"�޷������� config.cfg", L"����ʧ��", MB_OK | MB_ICONERROR);
		exit(-1);
	}
	std::stringstream str_stream;
	str_stream << file.rdbuf();
	str_address = str_stream.str();
	file.close();
}

void login_to_server(HWND hwnd)// ��¼��������
{
	client = new httplib::Client(str_address);
	client->set_keep_alive(true);

	httplib::Result result = client->Post("/login");
	if (!result || result->status != 200)
	{
		MessageBox(hwnd, _T("�޷����ӵ���������"), _T("����ʧ��"), MB_OK | MB_ICONERROR);
		exit(-1);
	}

	player_id = std::stoi(result->body);

	if (player_id <= 0)
	{
		MessageBox(hwnd, _T("�����Ѿ���ʼ����"), _T("�ܾ�����"), MB_OK | MB_ICONERROR);
		exit(-1);
	}

	std::thread([&]()
		{
			while (true)
			{
				using namespace std::chrono;

				std::string route = "/update";
				httplib::Result result = client->Post(route);

				if (result && result->status == 200)
				{
					cJSON* json_plate = cJSON_Parse(result->body.c_str());
					if (json_plate)
					{
						for (int i = 0; i < 10; ++i)
						{
							cJSON* item = cJSON_GetArrayItem(json_plate, i);
							if (item)
							{
								plate[i] = item->valueint;
							}
						}
						cJSON_Delete(json_plate);

					}
				}
				std::this_thread::sleep_for(nanoseconds(1000000000 / 10));

			}
		}).detach();
}

void draw_plate()// ��������
{
	setlinecolor(RGB(255, 255, 255));
	setlinestyle(PS_SOLID, 2);
	line(240, 0, 240, 720);
	line(480, 0, 480, 720);
	line(0, 240, 720, 240);
	line(0, 480, 720, 480);

	for (int i = 0; i < 9; ++i)
	{
		int x_center = 240 * (i % 3) + 120; // ����������ĵ�x����
		int y_center = 240 * (i / 3) + 120; // ����������ĵ�y����

		if (plate[i] == 1)
		{
			setlinecolor(RGB(255, 0, 0));
			setlinestyle(PS_SOLID, 2);
			circle(x_center, y_center, 60); // ����Բ�İ뾶��λ��
		}
		else if (plate[i] == 2)
		{
			setlinecolor(RGB(0, 0, 255));
			setlinestyle(PS_SOLID, 2);
			line(x_center - 40, y_center - 40, x_center + 40, y_center + 40); // ��������λ��
			line(x_center + 40, y_center - 40, x_center - 40, y_center + 40); // ��������λ��
		}
	}
}




