#include <iostream>
using namespace std;

// ע�⣺��������ΪURL�����ҷ�Ӣ���ַ�ΪUTF-8 
int main(int argc, char* argv[])
{

    cout << "Content-type:text/html\r\n\r\n";
    cout << "<html>\n";
    cout << "<head>\n";
    cout << "<title>Hello World - First CGI Program</title>\n";
    cout << "</head>\n";
    cout << "<body>\n";
    cout << "<h1>Hello World! </h1><br><h2>This is my first CGI program</h2>\n";

    if (argc>1)
    {
        for (int i=1; i<argc; i++)
        {
            cout<< "<h3>arg("<<i<<"): "<<argv[i]<<"</h3>\n";
        }
    }
    cout << "</body>\n";
    cout << "</html>\n";

    return 0;
}