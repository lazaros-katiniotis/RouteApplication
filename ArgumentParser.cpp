#include "ArgumentParser.h"
#include <cstdlib>
#include <iostream>
#include <charconv>
#include "Helper.h"

using namespace route_app;
using namespace std;

ArgumentParser::ArgumentParser() {
	Initialize();
}

void ArgumentParser::Initialize() {
	current_parser_state_ = ParserState::START_STATE;
	previous_parser_state_ = current_parser_state_;
	current_input_state_ = InputState::INVALID;
	parsing_type_ = InputState::INVALID;
	previous_input_state_ = current_input_state_;
	bound_query_ = "";
	number_of_coordinates_to_parse = -1;
	is_parsing_coordinates = false;
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
	cout << "currentParserState: " << (int)current_parser_state_ << ", currentInputState: " << (int)current_input_state_ << endl;
	UpdateInputState(arg);
	if (!ValidateParsingState(arg)) {
		return ParserState::ERROR_STATE;
	}
	ResetParsingState();

	UpdateParserState(arg);
	cout << "new currentParserState: " << (int)current_parser_state_ << ", new currentInputState: " << (int)current_input_state_ << endl;

	ParseCoordinates(arg);

	//query_ += arg;
	cout << endl;

	//UpdatePreviousStates();
	return current_parser_state_;
}

void ArgumentParser::UpdateInputState(string_view arg) {
	cout << "Parsing argument: " << arg << endl;

	if (previous_input_state_ != current_input_state_) {
		previous_input_state_ = current_input_state_;
	}

	if (arg == "-b") {
		current_input_state_ = InputState::BOUNDS_COMMAND;
	}
	else if (arg == "-f") {
		current_input_state_ = InputState::FILE_COMMAND;
	}
	else if (arg == "-start") {
		current_input_state_ = InputState::START_POINT_COMMAND;
	}
	else if (arg == "-end") {
		current_input_state_ = InputState::END_POINT_COMMAND;
	}
	else if (arg == "-p") {
		current_input_state_ = InputState::POINT_COMMAND;
	}
	else {
		double result;
		if (auto [p, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), result); ec == std::errc()) {
			current_input_state_ = InputState::COORDINATE;
		}
		else {
			current_input_state_ = InputState::FILENAME;
		}
	}

	//cout << "previousInputState:\t" << (int)previousInputState_ << endl;
	cout << "currentInputState:\t" << (int)current_input_state_ << endl;
}

void ArgumentParser::UpdateParserState(string_view arg) {
	if (!is_parsing_coordinates) {
		if (number_of_coordinates_to_parse < 0) {
			previous_parser_state_ = current_parser_state_;
			current_parser_state_ = static_cast<ParserState>(stateTable_->GetState(current_parser_state_, current_input_state_));
			//cout << "previousParserState: " << (int)previousParserState_ << endl;
			//cout << "currentParserState:\t" << (int)currentParserState_ << endl;
		}
	}

	switch (current_parser_state_) {
	case ParserState::ERROR_STATE:
		cout << "Error parsing arguments: " << arg << endl;
		break;
	case ParserState::OK_STATE:
		cout << "Parsed arguments successfully." << endl;
		break;
	case ParserState::PARSING_STATE:
		switch (previous_input_state_) {
		case InputState::BOUNDS_COMMAND:
			is_parsing_coordinates = true;
			number_of_coordinates_to_parse = bound_coordinates_;
			cout << "Will need to parse the next " << number_of_coordinates_to_parse << " coordinates." << endl;
			break;
		case InputState::START_POINT_COMMAND:
			is_parsing_coordinates = true;
			number_of_coordinates_to_parse = point_coordinates_;
			cout << "Will need to parse the next " << point_coordinates_ << " coordinates." << endl;
			break;
		case InputState::END_POINT_COMMAND:
			is_parsing_coordinates = true;
			number_of_coordinates_to_parse = point_coordinates_;
			cout << "Will need to parse the next " << point_coordinates_ << " coordinates." << endl;
			break;
		case InputState::POINT_COMMAND:
			is_parsing_coordinates = true;
			number_of_coordinates_to_parse = point_coordinates_;
			cout << "Will need to parse the next " << point_coordinates_ << " coordinates." << endl;
			break;
		case InputState::FILE_COMMAND:
			break;
		default:
			break;
		}
		break;
	}
}

bool ArgumentParser::ValidateParsingState(string_view arg) {
	switch (parsing_type_) {
	case InputState::BOUNDS_COMMAND:
		//if (is_parsing_coordinates) {
		if (number_of_coordinates_to_parse > 0 && current_input_state_ != InputState::COORDINATE) {
			cout << "Error parsing arguments: was expecting a coordinate as an argument. Instead, parsed '" << arg << "'." << endl;
			current_parser_state_ = ParserState::ERROR_STATE;
			return false;
		}
		//}
		break;
	default:
		break;
	}
	return true;
}

void ArgumentParser::ResetParsingState() {
	switch (parsing_type_) {
	case InputState::BOUNDS_COMMAND:
		//if (is_parsing_coordinates) {
		if (number_of_coordinates_to_parse == 0) {
			cout << "Reseting currentParserState to START_STATE." << endl;
			current_parser_state_ = ParserState::START_STATE;
			parsing_type_ = InputState::INVALID;
			is_parsing_coordinates = false;
			number_of_coordinates_to_parse = -1;
		}
		//}
		break;
	case InputState::FILE_COMMAND:
		cout << "Reseting currentParserState to START_STATE." << endl;
		current_parser_state_ = ParserState::START_STATE;
		parsing_type_ = InputState::INVALID;
		is_parsing_coordinates = false;
		break;
	}

}

void ArgumentParser::ParseCoordinates(string_view arg) {
	cout << "current_type_: " << (int)parsing_type_ << endl;
	if (parsing_type_ == InputState::INVALID) {
		if (previous_input_state_ != InputState::COORDINATE && previous_input_state_ != InputState::FILENAME) {
			parsing_type_ = previous_input_state_;
			cout << "new current_type_: " << (int)parsing_type_ << endl;
		}
	}

	switch (parsing_type_) {
	case InputState::BOUNDS_COMMAND:
		//if (is_parsing_coordinates) {
			if (current_parser_state_ == ParserState::PARSING_STATE && current_input_state_ == InputState::COORDINATE) {
				bound_query_ += arg;
				number_of_coordinates_to_parse--;
				cout << "Added '" << arg << "' to the query." << endl;
				cout << "number_of_coordinates_left_to_parse: " << number_of_coordinates_to_parse << endl;
				if (number_of_coordinates_to_parse != 0) {
					bound_query_ += ",";
				}
			}
		//}
		break;
	case InputState::FILE_COMMAND:
		cout << "FILENAME: " << arg << endl;
		filename_ = arg;
	default:
		break;
	}
}

void ArgumentParser::UpdatePreviousStates() {
	previous_parser_state_ = current_parser_state_;
	previous_input_state_ = current_input_state_;
}

string ArgumentParser::GetBoundQuery() const {
	return bound_query_;
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