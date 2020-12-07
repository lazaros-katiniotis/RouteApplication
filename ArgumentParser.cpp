#include "ArgumentParser.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <charconv>
#include "Helper.h"

using namespace route_app;
using namespace std;

ArgumentParser::ArgumentParser() {
	Initialize();
}

void ArgumentParser::Initialize() {
	Reset();
	previous_parser_state_ = current_parser_state_;
	current_input_state_ = InputState::INVALID;
	previous_input_state_ = current_input_state_;
	bound_query_ = "";
	CreateStateTable();
}

void ArgumentParser::Reset() {
	current_parser_state_ = ParserState::START_STATE;
	parsing_type_ = InputState::INVALID;
	number_of_coordinates_to_parse = -1;
	coords_.clear();
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
	if (current_parser_state_ = ValidateParsingState(arg); current_parser_state_ != ParserState::ERROR_STATE) {
		UpdateParserState(arg);
		cout << "new currentParserState: " << (int)current_parser_state_ << ", new currentInputState: " << (int)current_input_state_ << endl;
		StoreData(arg);
		ResetParsingState();
	}
	cout << endl;
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
			coords_.emplace_back(result);
		}
		else {
			current_input_state_ = InputState::FILENAME;
		}
	}

	//cout << "previousInputState:\t" << (int)previousInputState_ << endl;
	cout << "currentInputState:\t" << (int)current_input_state_ << endl;
}

void ArgumentParser::UpdateParserState(string_view arg) {
	//if (parsing_type_ == InputState::INVALID) {
		if (number_of_coordinates_to_parse < 0) {
			previous_parser_state_ = current_parser_state_;
			current_parser_state_ = static_cast<ParserState>(stateTable_->GetState(current_parser_state_, current_input_state_));
			//cout << "previousParserState: " << (int)previousParserState_ << endl;
			//cout << "currentParserState:\t" << (int)currentParserState_ << endl;
		}
	//}

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
			number_of_coordinates_to_parse = bound_coordinates_;
			cout << "Will need to parse the next " << number_of_coordinates_to_parse << " coordinates." << endl;
			break;
		case InputState::START_POINT_COMMAND:
			number_of_coordinates_to_parse = point_coordinates_;
			cout << "Will need to parse the next " << point_coordinates_ << " coordinates." << endl;
			break;
		case InputState::END_POINT_COMMAND:
			number_of_coordinates_to_parse = point_coordinates_;
			cout << "Will need to parse the next " << point_coordinates_ << " coordinates." << endl;
			break;
		case InputState::POINT_COMMAND:
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

ArgumentParser::ParserState ArgumentParser::ValidateParsingState(string_view arg) {
	switch (parsing_type_) {
	case InputState::BOUNDS_COMMAND:
		if (number_of_coordinates_to_parse > 0 && current_input_state_ != InputState::COORDINATE) {
			cout << "Error parsing bound arguments: was expecting a coordinate as an argument. Instead, parsed '" << arg << "'." << endl;
			return ParserState::ERROR_STATE;
		}
		break;
	case InputState::START_POINT_COMMAND:
		if (number_of_coordinates_to_parse > 0 && current_input_state_ != InputState::COORDINATE) {
			cout << "Error parsing start arguments: was expecting a coordinate as an argument. Instead, parsed '" << arg << "'." << endl;
			return ParserState::ERROR_STATE;
		}
		break;
	case InputState::END_POINT_COMMAND:
		if (number_of_coordinates_to_parse > 0 && current_input_state_ != InputState::COORDINATE) {
			cout << "Error parsing end arguments: was expecting a coordinate as an argument. Instead, parsed '" << arg << "'." << endl;
			return ParserState::ERROR_STATE;
		}
		break;
	case InputState::POINT_COMMAND:
		if (number_of_coordinates_to_parse > 0 && current_input_state_ != InputState::COORDINATE) {
			cout << "Error parsing point arguments: was expecting a coordinate as an argument. Instead, parsed '" << arg << "'." << endl;
			return ParserState::ERROR_STATE;
		}
		break;
	case InputState::FILE_COMMAND:
		if (current_input_state_ != InputState::FILENAME) {
			cout << "Error parsing filename argument: was expecting a filename as an argument. Instead, parsed '" << arg << "'." << endl;
			return ParserState::ERROR_STATE;
		}
		break;
	default:
		break;
	}
	return current_parser_state_;
}

void ArgumentParser::ResetParsingState() {
	switch (parsing_type_) {
	case InputState::BOUNDS_COMMAND:
		if (number_of_coordinates_to_parse == 0) {
			cout << "Storing Bound Data" << endl;
			for (int i = 0; i < 3; i++) {
				bound_query_ += to_string(coords_[i]) + ",";
			}
			bound_query_ += to_string(coords_[3]);
			Reset();
		}
		break;
	case InputState::START_POINT_COMMAND:
		if (number_of_coordinates_to_parse == 0) {
			cout << "Storing Start Data" << endl;
			InitializePoint(starting_point_);
			Reset();
		}
		break;
	case InputState::END_POINT_COMMAND:
		if (number_of_coordinates_to_parse == 0) {
			cout << "Storing end Data" << endl;
			InitializePoint(ending_point_);
			Reset();
		}
		break;
	case InputState::POINT_COMMAND:
		if (number_of_coordinates_to_parse == 0) {
			cout << "Storing point Data" << endl;
			InitializePoint(point_);
			Reset();
		}
		break;
	case InputState::FILE_COMMAND:
		Reset();
		break;
	default:
		break;
	}

}

void inline ArgumentParser::InitializePoint(Model::Node& node) {
	node.x = coords_[0];
	node.y = coords_[1];
}

void ArgumentParser::StoreData(string_view arg) {
	cout << "current_type_: " << (int)parsing_type_ << endl;
	cout << "previous_parser_state_: " << (int)previous_parser_state_ << endl;
	if (parsing_type_ == InputState::INVALID) {
		if (previous_input_state_ != InputState::COORDINATE && previous_input_state_ != InputState::FILENAME) {
			parsing_type_ = previous_input_state_;
			cout << "new current_type_: " << (int)parsing_type_ << endl;
		}
	}

	switch (parsing_type_) {
	case InputState::BOUNDS_COMMAND:
		number_of_coordinates_to_parse--;
		cout << "Added '" << arg << "' to the query." << endl;
		cout << "number_of_coordinates_left_to_parse: " << number_of_coordinates_to_parse << endl;
		break;
	case InputState::FILE_COMMAND:
		filename_ = arg;
		cout << "FILENAME: " << arg << endl;
		break;
	case InputState::START_POINT_COMMAND:
		number_of_coordinates_to_parse--;
		cout << "Storing start data..." << endl;
		break;
	case InputState::END_POINT_COMMAND:
		number_of_coordinates_to_parse--;
		cout << "Storing end data..." << endl;
		break;
	case InputState::POINT_COMMAND:
		number_of_coordinates_to_parse--;
		cout << "Storing point data..." << endl;
		break;
	default:
		break;
	}
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