#pragma once
#include <vector>
#include <string>
#include "TypicalValue.h"

namespace qualitymeasurement {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for MainForm
	/// </summary>
	public ref class MainForm : public System::Windows::Forms::Form
	{
		enum class Items : int
		{
			SNR,
			CTF,
			CNR
		};

		enum class Area : int
		{
			LEFT = 2,
			RIGHT,
			AVERAGE
		};

	public:
		MainForm(void);

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~MainForm();

	private: System::Windows::Forms::PictureBox^ pictureBox;
	protected:
	private: System::Windows::Forms::Panel^ panel1;
	private: System::Windows::Forms::Label^ strWight;
	private: System::Windows::Forms::Label^ strXY;
	private: System::Windows::Forms::Label^ strHeight;
	private: System::Windows::Forms::Label^ label9;
	private: System::Windows::Forms::Label^ label3;
	private: System::Windows::Forms::Label^ label2;
	private: System::Windows::Forms::Button^ btnExportResult;
	private: System::Windows::Forms::Button^ btnApplyAll;
	private: System::Windows::Forms::Button^ btnPrevious;
	private: System::Windows::Forms::Button^ btnNext;
	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;
	private:
		String^						xmlFile = "config.xml";
		TypicalValue^				typicalValue;
		System::Drawing::Image^		m_image;
		array<System::String^>^		fnSystem;
		System::Drawing::Rectangle	m_rect;
		System::Drawing::Rectangle	m_rectLeft;
		System::Drawing::Rectangle	m_rectRight;
		System::Drawing::Point		m_centerPanel;
		System::Drawing::Point		m_offsetPoint;
		System::Drawing::Rectangle	m_picBoxOrgSize;
		bool				m_isMouseDown;
		float				m_scale;
		bool				m_isDraw;
		bool				m_isMove;
		bool				m_isMoving;
		int					m_imageIdx;
		int					m_totalImage;
		const int			m_cum_width = 300;
		const int			m_cum_height = 130;
		bool				m_calculateLUT = true;
		int					rotateFlag = 3;
		bool				m_rotateUpdate = false;
		int m_mouseInsideCorner = -1;
	private: System::Windows::Forms::Button^ btnMove;
	private: System::Windows::Forms::Label^ strIndex;
	private: System::Windows::Forms::Panel^ panel2;
	private: System::Windows::Forms::Panel^ panel3;
	private: System::Windows::Forms::Label^ label14;
	private: System::Windows::Forms::Label^ label13;
	private: System::Windows::Forms::Label^ label12;
	private: System::Windows::Forms::Label^ label11;
	private: System::Windows::Forms::PictureBox^ picBoxCNRRight;
	private: System::Windows::Forms::PictureBox^ picBoxCNRLeft;
	private: System::Windows::Forms::PictureBox^ picBoxCTFRight;
	private: System::Windows::Forms::PictureBox^ picBoxCTFLeft;
	private: System::Windows::Forms::Label^ label10;
	private: System::Windows::Forms::Label^ label8;
	private: System::Windows::Forms::DataGridView^ dataGridView1;

	private: System::Windows::Forms::Label^ label7;
	private: System::Windows::Forms::TextBox^ textBox1;
	private: System::Windows::Forms::Label^ label5;
	private: System::Windows::Forms::Button^ btnBrown1;
	private: System::Windows::Forms::VScrollBar^ vScrollBar1;
	private: System::Windows::Forms::HScrollBar^ hScrollBar1;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ Column1;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ Column2;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ Column3;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ Column4;
	private: System::Windows::Forms::DataGridViewTextBoxColumn^ Column5;
	private: System::Windows::Forms::Button^ bt_ExportTMO;
	private: System::Windows::Forms::PictureBox^ picBoxCumulative;
	private: System::Windows::Forms::Label^ label1;
	private: System::Windows::Forms::TextBox^ textBox3;
	private: System::Windows::Forms::TextBox^ textBox2;
private: System::Windows::Forms::RadioButton^ radioButtonROICertainArea;
private: System::Windows::Forms::RadioButton^ radioButtonROI2Middle;
private: System::Windows::Forms::Label^ label4;
private: System::Windows::Forms::Label^ label6;

	private: System::Windows::Forms::Button^ btnOpenFile;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void);
		void Update();
		void Reset();
		void caculateScaleOffset();
		Bitmap^ GraphDraw(System::Drawing::Graphics^ grap, std::vector<int> xCoordinate, std::vector<double> list, std::string title, std::string lable, double typical);
		Bitmap^ GraphDraw(System::Drawing::Graphics^ grap, std::vector<double> list, std::string title, std::string lable);
		void LoadTypicalValue();
	private: System::Void pictureBox_MouseDown(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e);
	private: System::Void pictureBox_MouseMove(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e);
	private: System::Void pictureBox_MouseUp(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e);
	private: System::Void pictureBox_Paint(System::Object^ sender, System::Windows::Forms::PaintEventArgs^ e);
#pragma endregion
	private: System::Void pictureBox_MouseWheel(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e);
	private: System::Void btnMove_Click(System::Object^ sender, System::EventArgs^ e);
	private: System::Void textBox1_TextChanged(System::Object^ sender, System::EventArgs^ e);
	private: System::Void btnNext_Click(System::Object^ sender, System::EventArgs^ e);
	private: System::Void btnPrevious_Click(System::Object^ sender, System::EventArgs^ e);
	private: System::Void btnBrown1_Click(System::Object^ sender, System::EventArgs^ e);
	private: System::Void btnExportResult_Click(System::Object^ sender, System::EventArgs^ e);
	private: System::Void btnApplyAll_Click(System::Object^ sender, System::EventArgs^ e);
	private: System::Void dataGridView1_CellEndEdit(System::Object^ sender, System::Windows::Forms::DataGridViewCellEventArgs^ e);
	private: System::Void btnOpenFile_Click(System::Object^ sender, System::EventArgs^ e);
	private: System::Void vScrollBar1_ValueChanged(System::Object^ sender, System::EventArgs^ e);
	private: System::Void hScrollBar1_ValueChanged(System::Object^ sender, System::EventArgs^ e);
	private: System::Void bt_ExportTMO_Click(System::Object^ sender, System::EventArgs^ e);
	private: System::Void arrow_KeyDown(System::Object^ sender, System::Windows::Forms::KeyEventArgs^ e);
	private:bool isMouseInsideRect(System::Drawing::Rectangle rect, System::Windows::Forms::MouseEventArgs^ e);
private: System::Void radioButton2_CheckedChanged(System::Object^ sender, System::EventArgs^ e)
{
	Update();
}
private: System::Void radioButton1_CheckedChanged(System::Object^ sender, System::EventArgs^ e)
{
	Update();
}
};
}
