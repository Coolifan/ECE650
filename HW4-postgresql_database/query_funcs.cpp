#include "query_funcs.h"


void add_player(connection *C, int team_id, int jersey_num, string first_name, string last_name,
                int mpg, int ppg, int rpg, int apg, double spg, double bpg)
{
	stringstream sql;
	work W(*C);
	sql << "INSERT INTO PLAYER (TEAM_ID,UNIFORM_NUM,FIRST_NAME,LAST_NAME,MPG,PPG,RPG,APG,SPG,BPG) "
		<< "VALUES (" << team_id << "," << jersey_num << ",'" << W.esc(first_name) << "','"  << W.esc(last_name) 
		<< "'," << mpg << "," << ppg << "," << rpg << "," << apg <<"," << spg << "," << bpg << ");"; 
	W.exec(sql.str());
	W.commit();
}


void add_team(connection *C, string name, int state_id, int color_id, int wins, int losses)
{
	stringstream sql;
	work W(*C);
	sql << "INSERT INTO TEAM (NAME, STATE_ID, COLOR_ID, WINS, LOSSES) "
		<< "VALUES ('" << W.esc(name) << "'," << state_id << "," << color_id << ","
		<< wins << "," << losses << ");"; 
	W.exec(sql.str());
	W.commit();
}


void add_state(connection *C, string name)
{
	stringstream sql;
	work W(*C);
	sql << "INSERT INTO STATE (NAME) " 
		<< "VALUES ('" << W.esc(name) << "');";
	
	W.exec(sql.str());
	W.commit();
}


void add_color(connection *C, string name)
{	
	work W(*C);
	stringstream sql;
	sql << "INSERT INTO COLOR (NAME) " 
		<< "VALUES ('" << W.esc(name) << "');";
	
	W.exec(sql.str());
	W.commit();
}


void query1(connection *C,
	    int use_mpg, int min_mpg, int max_mpg,
        int use_ppg, int min_ppg, int max_ppg,
		int use_rpg, int min_rpg, int max_rpg,
		int use_apg, int min_apg, int max_apg,
		int use_spg, double min_spg, double max_spg,
        int use_bpg, double min_bpg, double max_bpg
            )
{
	// Create SQL statement
  	stringstream sql;
	// show all attributes of each player with average statistics that fall
	// between the min and max (inclusive) for each enabled statistic 
  	int use[6] = {use_mpg, use_ppg, use_rpg, use_apg, use_spg, use_bpg};
  	const char * stats[6] = {"MPG", "PPG", "RPG", "APG", "SPG", "BPG"};
  	double min[6] = {min_mpg, min_ppg, min_rpg, min_apg, min_spg, min_bpg};
  	double max[6] = {max_mpg, max_ppg, max_rpg, max_apg, max_spg, max_bpg};

  	sql << "SELECT * FROM PLAYER ";
  	int cnt = 0;
  	for (int i = 0; i < 6; i++) {
  		if (use[i] && cnt == 0) {
  			sql << "WHERE ";
  		}
  		if (use[i]) {
  			sql << stats[i] << " >= " << min[i] << " AND " << stats[i] << " <= " << max[i] << " ";
  			cnt++;
  		}
  		if (i != 5 && use[i+1] != 0) {
  			sql << "AND ";
  		}
  	}
  	sql << ";";

  	// Create a non-transactional object
  	nontransaction N(*C);
  	// list down all the results
  	result R(N.exec(sql.str()));
  	cout << "PLAYER_ID TEAM_ID UNIFORM_NUM FIRST_NAME LAST_NAME MPG PPG RPG APG SPG BPG" << endl;
  	for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
  		cout << c[0].as<int>() << " " << c[1].as<int>() << " " << c[2].as<int>() << " " <<
  				c[3].as<string>() << " " << c[4].as<string>() << " " << c[5].as<int>() << " " <<
  				c[6].as<int>() << " " << c[7].as<int>() << " " << c[8].as<int>() << " " <<
  				c[9].as<double>() << " " << c[10].as<double>() << endl;
  	}
}


void query2(connection *C, string team_color)
{
	stringstream sql;
	//  show the name of each team with the indicated uniform color 
	sql << "SELECT T.NAME " \
		<< "FROM TEAM AS T, COLOR AS C "\
		<< "WHERE C.NAME='" << team_color << "' AND C.COLOR_ID=T.COLOR_ID;";

	nontransaction N(*C);
	result R(N.exec(sql.str()));
	cout << "NAME" << endl;
	for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
		cout << c[0].as<string>() << endl;
	}
}


void query3(connection *C, string team_name)
{
	stringstream sql;
	// show the first and last name of each player that plays for the indicated
	// team, ordered from highest to lowest ppg (points per game) 
	sql << "SELECT FIRST_NAME, LAST_NAME " \
		<< "FROM PLAYER, TEAM " \
		<< "WHERE TEAM.NAME='" << team_name << "' AND TEAM.TEAM_ID=PLAYER.TEAM_ID ORDER BY PPG DESC;"; 

	nontransaction N(*C);
	result R(N.exec(sql.str()));
	cout << "FIRST_NAME LAST_NAME" << endl;
	for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
		cout << c[0].as<string>() << " " << c[1].as<string>() << endl;
	}
}


void query4(connection *C, string team_state, string team_color)
{
	// 	show first name, last name, and jersey number of each player that
	// plays in the indicated state and wears the indicated uniform color
	stringstream sql;
	sql << "SELECT P.FIRST_NAME, P.LAST_NAME, P.UNIFORM_NUM " \
		<< "FROM PLAYER AS P, TEAM AS T, STATE AS S, COLOR AS C " \
		<< "WHERE C.NAME='" << team_color << "' AND S.NAME='" \
			<< team_state << "' AND C.COLOR_ID=T.COLOR_ID AND S.STATE_ID=T.STATE_ID AND T.TEAM_ID=P.TEAM_ID;";

	nontransaction N(*C);
	result R(N.exec(sql.str()));
	cout << "FIRST_NAME LAST_NAME UNIFORM_NUM" << endl;
	for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
		cout << c[0].as<string>() << " " << c[1].as<string>() << " " << c[2].as<int>() << endl;
	}
}


void query5(connection *C, int num_wins)
{
	// show first name and last name of each player, and team name and
	// number of wins for each team that has won more than the indicated number of
	// games
	stringstream sql;
	sql << "SELECT FIRST_NAME, LAST_NAME, NAME, WINS "\
	 	<< "FROM PLAYER, TEAM "\
	 	<< "WHERE WINS>" << num_wins << " AND TEAM.TEAM_ID=PLAYER.TEAM_ID;";

	nontransaction N(*C);
	result R(N.exec(sql.str()));
	cout << "FIRST_NAME LAST_NAME NAME WINS" << endl;
	for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
		cout << c[0].as<string>() << " " << c[1].as<string>() << " " << c[2].as<string>() << " " << c[3].as<int>() << endl;
	}
}
