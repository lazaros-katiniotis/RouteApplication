#include "ArgumentParser.h"
#include <cstdlib>
#include <iostream>
#include <charconv>

using namespace route_app;
using namespace std;

ArgumentParser::ArgumentParser() {
	Initialize();
}

void ArgumentParser::Initialize() {
	currentParserState_ = ParserState::START_STATE;
	previousParserState_ = currentParserState_;
	currentInputState_ = InputState::INVALID;
	previousInputState_ = currentInputState_;
	query_ = "";
	CreateStateTable();
}


void ArgumentParser::CreateStateTable() {
	stateTable_ = new StateTable();
	for (int i = 0; i < stateTable_->GetWidth(); i++) {
		for (int j = 0; j < stateTable_->GetHeight(); j++) {
			stateTable_->SetState(i, j, ParserState::ERROR_STATE);
		}
	}
	stateTable_->SetState(ParserState::START_STATE, InputState::BOUNDS_COMMAND, ParserState::BOUNDS_STATE);
	stateTable_->SetState(ParserState::START_STATE, InputState::FILE_COMMAND, ParserState::FILE_STATE);
	stateTable_->SetState(ParserState::START_STATE, InputState::START_POINT_COMMAND, ParserState::POINT_STATE);
	stateTable_->SetState(ParserState::START_STATE, InputState::END_POINT_COMMAND, ParserState::POINT_STATE);
	stateTable_->SetState(ParserState::START_STATE, InputState::POINT_COMMAND, ParserState::POINT_STATE);

	stateTable_->SetState(ParserState::BOUNDS_STATE, InputState::COORDINATE, ParserState::PARSING_STATE);

	stateTable_->SetState(ParserState::POINT_STATE, InputState::COORDINATE, ParserState::PARSING_STATE);

	stateTable_->SetState(ParserState::FILE_STATE, InputState::FILENAME, ParserState::PARSING_STATE);
}

ArgumentParser::ParserState ArgumentParser::ParseArgument(string_view arg) {
	UpdateInputState(arg);
	UpdateParserState(arg);
	UpdatePreviousStates();
	return currentParserState_;
}

void ArgumentParser::UpdateInputState(string_view arg) {
	cout << "currentParserState: " << (int)currentParserState_ << ", currentInputState: " << (int)currentInputState_ << endl;
	cout << "Parsing argument: " << arg << endl;
	
	if (arg == "-b") {
		currentInputState_ = InputState::BOUNDS_COMMAND;
	}
	else if (arg == "-f") {
		currentInputState_ = InputState::FILE_COMMAND;
	}
	else if (arg == "-start") {
		currentInputState_ = InputState::START_POINT_COMMAND;
	}
	else if (arg == "-end") {
		currentInputState_ = InputState::END_POINT_COMMAND;
	}
	else if (arg == "-p") {
		currentInputState_ = InputState::POINT_COMMAND;
	}
	else {
		double result;
		if (auto [p, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), result); ec == std::errc()) {
			currentInputState_ = InputState::COORDINATE;
		}
		else {
			currentInputState_ = InputState::FILENAME;
		}
	}
	cout << "updated currentInputState:\t" << (int)currentInputState_ << endl;
}

void ArgumentParser::UpdateParserState(string_view arg) {
	static int number_of_coordinates_to_parse = -1;
	static bool parsing_coordinates = false;

	if (parsing_coordinates) {
		if (number_of_coordinates_to_parse > 0 && currentInputState_ != InputState::COORDINATE) {
			cout << "Error reading arguments: was expecting a coordinate as an argument." << endl;
			currentParserState_ = ParserState::ERROR_STATE;
			return;
		}

		if (number_of_coordinates_to_parse == 0) {
			cout << "Reseting currentParserState to START_STATE." << endl;
			currentParserState_ = ParserState::START_STATE;
			parsing_coordinates = false;
			number_of_coordinates_to_parse = -1;
		}
	}
	if (!parsing_coordinates) {
		if (number_of_coordinates_to_parse < 0) {
			currentParserState_ = static_cast<ParserState>(stateTable_->GetState(currentParserState_, currentInputState_));
			cout << "updated currentParserState:\t" << (int)currentParserState_ << endl;
		}
	}

	switch (currentParserState_) {
	case ParserState::ERROR_STATE:
		cout << "Error parsing arguments: " << arg << endl;
		break;
	case ParserState::OK_STATE:
		cout << "Parsed arguments successfully." << endl;
		break;
	case ParserState::PARSING_STATE:
		switch (previousInputState_) {
		case InputState::BOUNDS_COMMAND:
			parsing_coordinates = true;
			number_of_coordinates_to_parse = bound_coordinates_;
			cout << "Will need to parse the next " << number_of_coordinates_to_parse << " coordinates." << endl;
			break;
		case InputState::START_POINT_COMMAND:
			parsing_coordinates = true;
			number_of_coordinates_to_parse = point_coordinates_;
			cout << "Will need to parse the next " << point_coordinates_ << " coordinates." << endl;
			break;
		case InputState::END_POINT_COMMAND:
			parsing_coordinates = true;
			number_of_coordinates_to_parse = point_coordinates_;
			cout << "Will need to parse the next " << point_coordinates_ << " coordinates." << endl;
			break;
		case InputState::POINT_COMMAND:
			parsing_coordinates = true;
			number_of_coordinates_to_parse = point_coordinates_;
			cout << "Will need to parse the next " << point_coordinates_ << " coordinates." << endl;
			break;
		case InputState::FILE_COMMAND:
			cout << "FILENAME: " << arg << endl;
			break;
		default:
			break;
		}
		break;
	}

	if (parsing_coordinates) {
		if (currentParserState_ == ParserState::PARSING_STATE && currentInputState_ == InputState::COORDINATE) {
			query_ += arg;
			number_of_coordinates_to_parse--;
			cout << "Added '" << arg << "' to the query." << endl;
			cout << "number_of_coordinates_left_to_parse: " << number_of_coordinates_to_parse << endl;
			if (number_of_coordinates_to_parse != 0) {
				query_ += ",";
			}
		}
	}

	//query_ += arg;
	cout << endl;
}

void ArgumentParser::UpdatePreviousStates() {
	previousParserState_ = currentParserState_;
	previousInputState_ = currentInputState_;
}

string ArgumentParser::GetQuery() {
	return query_;
}

void ArgumentParser::Release() {
	delete stateTable_;
}

ArgumentParser::~ArgumentParser() {
	Release();
}

size_t inline ArgumentParser::StateTable::Index(int x, int y) const {
	return x + width_ * y;
}

void inline ArgumentParser::StateTable::SetState(int x, int y, ParserState state) const {
	array[Index(x, y)] = (int)state;
}
void inline ArgumentParser::StateTable::SetState(ParserState x, InputState y, ParserState state) const {
	array[Index((int)x, (int)y)] = (int)state;
}

int inline ArgumentParser::StateTable::GetState(int x, int y) const {
	return array[Index(x, y)];
}

int inline ArgumentParser::StateTable::GetState(ParserState x, InputState y) const {
	return array[Index((int)x, (int)y)];
}