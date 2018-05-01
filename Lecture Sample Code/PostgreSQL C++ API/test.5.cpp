#include <iostream>
#include <pqxx/pqxx> 
//#include <result>

using namespace std;
//using namespace pqxx;

int main(int argc, char* argv[])
{
  string sql;
   
  try{
    pqxx::connection C("dbname=testdb user=postgres password=abc123");
    if (C.is_open()) {
      cout << "Opened database successfully: " << C.dbname() << endl;
    } else {
      cout << "Can't open database" << endl;
      return 1;
    }
    /* Create SQL statement */
    sql = "SELECT * from COMPANY";

    /* Create a non-transactional object. */
    pqxx::nontransaction N(C);
      
    /* Execute SQL query */
    pqxx::result R =  N.exec( sql );
      
    /* List down all the records */
    for (pqxx::result::const_iterator c = R.begin(); c != R.end(); ++c) {
      for (pqxx::tuple::const_iterator field = c->begin(); field != c->end(); ++field) {
	cout << field->c_str() << '\t';
      }
      cout << endl;
    }
    cout << "Operation done successfully" << endl;
    C.disconnect ();
  } catch (const std::exception &e){
    cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}
