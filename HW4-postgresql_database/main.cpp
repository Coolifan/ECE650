#include <iostream>
#include <pqxx/pqxx>
#include <fstream>
#include <sstream>
#include <string>
#include "exerciser.h"

using namespace std;
using namespace pqxx;

void drop_and_create_tables(connection *C) {
  string player, team, state, color;
  player = "CREATE TABLE PLAYER(" \
    "PLAYER_ID SERIAL PRIMARY KEY NOT NULL," \
    "TEAM_ID INT NOT NULL," \
    "UNIFORM_NUM INT NOT NULL," \
    "FIRST_NAME TEXT NOT NULL," \
    "LAST_NAME TEXT NOT NULL," \
    "MPG INT NOT NULL," \
    "PPG INT NOT NULL," \
    "RPG INT NOT NULL," \
    "APG INT NOT NULL," \
    "SPG REAL NOT NULL," \
    "BPG REAL NOT NULL );";

  team = "CREATE TABLE TEAM(" \
    "TEAM_ID SERIAL PRIMARY KEY NOT NULL," \
    "NAME TEXT NOT NULL," \
    "STATE_ID INT NOT NULL," \
    "COLOR_ID INT NOT NULL," \
    "WINS INT NOT NULL," \
    "LOSSES INT NOT NULL );";

  state = "CREATE TABLE STATE(" \
    "STATE_ID SERIAL PRIMARY KEY NOT NULL," \
    "NAME TEXT NOT NULL );";

  color = "CREATE TABLE COLOR(" \
    "COLOR_ID SERIAL PRIMARY KEY NOT NULL," \
    "NAME TEXT NOT NULL );";

  /* Create a transactional object*/
  work W(*C);
  
  string drop("DROP TABLE IF EXISTS PLAYER, TEAM, STATE, COLOR;");
  W.exec(drop);
  //cout<< "Existing tables dropped successfully" << endl;
  
  W.exec(player);
  W.exec(team);
  W.exec(state);
  W.exec(color);
  W.commit();

  //cout << "Tables created successfully" << endl;
}

void read_player_entries(connection *C) {
  // load player data
  ifstream playerS("player.txt", ifstream::in);
  int player_id, team_id, jersey_num, mpg, ppg, rpg, apg;
  double spg, bpg;
  string first_name, last_name;
                
  string buffer;
  while (getline(playerS, buffer)) {
    if (!buffer.empty()) {
      stringstream ss;
      ss << buffer;
      ss >> player_id >> team_id >> jersey_num >> first_name >> last_name >> mpg >> ppg >> rpg >> apg >> spg >> bpg;
      add_player(C, team_id, jersey_num, first_name, last_name, mpg, ppg, rpg, apg, spg, bpg);
    }
  }
  playerS.close();
}

void read_team_entries(connection *C) {
  //load team data
  ifstream teamS("team.txt", ifstream::in);
  int team_id, state_id, color_id, wins, losses;
  string name;

  string buffer;
  while (getline(teamS, buffer)) {
    if (!buffer.empty()) {
      stringstream ss;
      ss << buffer;
      ss >> team_id >> name >> state_id >> color_id >> wins >> losses;
      add_team(C, name, state_id, color_id, wins, losses);
    }
  }
  teamS.close();
}

void read_state_entries(connection *C) {
  // load state data
  ifstream stateS("state.txt", ifstream::in);
  int state_id;
  string name;

  string buffer;
  while (getline(stateS, buffer)) {
    if (!buffer.empty()) {
      stringstream ss;
      ss << buffer;
      ss >> state_id >> name;
      add_state(C, name);
    }
  }
  stateS.close();
}

void read_color_entries(connection *C) {
  // load color data
  ifstream colorS("color.txt", ifstream::in);
  int color_id;
  string name;

  string buffer;
  while (getline(colorS, buffer)) {
    if (!buffer.empty()) {
      stringstream ss;
      ss << buffer;
      ss >> color_id >> name;
      add_color(C, name);
    }
  }
  colorS.close();
}


int main (int argc, char *argv[]) 
{

  //Allocate & initialize a Postgres connection object
  connection *C;

  try{
    //Establish a connection to the database
    //Parameters: database name, user name, user password
    C = new connection("dbname=ACC_BBALL user=postgres password=passw0rd");
    if (C->is_open()) {
      cout << "Opened database successfully: " << C->dbname() << endl;
    } else {
      cout << "Can't open database" << endl;
      return 1;
    }
  } catch (const std::exception &e){
    cerr << e.what() << std::endl;
    return 1;
  }


  //TODO: create PLAYER, TEAM, STATE, and COLOR tables in the ACC_BBALL database
  //      load each table with rows from the provided source txt files
  try {
    drop_and_create_tables(C);
    read_player_entries(C);
    read_team_entries(C);
    read_state_entries(C);
    read_color_entries(C);
    //cout << "Files loaded successfully" << endl;    
  } catch (const std::exception &e) {
    cerr << e.what() << endl;
    return 1;
  }



  
  exercise(C);


  //Close database connection
  C->disconnect();

  return 0;
}


