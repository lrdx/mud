// by bodrich (2014)
// http://mud.ru

class Quest
{
    public:
	Quest(int id, int time_start, int time_end, const std::string& text_quest, const std::string& tquest, int var_quest);
	//~Quest();
	int get_id();
	int get_time_start();
	int get_time_end();
	int get_time();
	std::string get_text();
	std::string get_tquest();
	int get_var_quest();
	int pquest();
	void set_pvar(int pvar);
    private:
	// id ������
	int id;
	// �����, ����� ��� ���� �����
	int time_start;
	// �����, ����� ����� ����� �����
	int time_end;
	// �������� ������
	std::string text_quest;
	// ������� ��������
	std::string tquest;
	// ���������� ��� ������, ������� ���� ����� �����, �������� ������� � ��
	int var_quest;
	// ������� ��� ���������
	int pvar_quest;
};

// vim: ts=4 sw=4 tw=0 noet syntax=cpp :
