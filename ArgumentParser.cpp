#include "ArgumentParser.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <charconv>
#include "Helper.h"

using namespace route_app;
using namespace std;

ArgumentParser::ArgumentParser(const int& argc, char** argv) {
	Initialize(argc, argv);
}

void ArgumentParser::Initialize(const int& argc, char** argv) {
	PrintDebugMessage(APPLICATION_NAME, "ArgumentParser", "Initializing argument parser...", true);
	Reset();
	previous_parser_state_ = current_parser_state_;
	current_input_state_ = InputState::INVALID;
	previous_input_state_ = current_input_state_;
	bound_query_ = "";
	syntax_state_ = 0x00;
	CreateStateTable();
}

void ArgumentParser::Reset() {
	current_parser_state_ = ParserState::START_STATE;
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

	stateTable_->SetState(ParserState::PARSING_STATE, InputState::COORDINATE, ParserState::PARSING_STATE);
	stateTable_->SetState(ParserState::PARSING_STATE, InputState::FILENAME, ParserState::PARSING_STATE);
}

bool ArgumentParser::SyntaxAnalysis(const int& argc, char** argv) {
	if (argc == 1) {
		DefaultSyntaxExample();
		return true;
	}

	for (int i = 1; i < argc; i++) {
		if (ParseArgument(argv[i]) == ParserState::ERROR_STATE) {
			return !CheckForWrongArgumentError(argc, argv, i);
		}

		if (CheckForMissingArgumentError(argc, argv, i)) {
			return false;
		}

		while (number_of_coordinates_to_parse >= 0) {
			if (ParseArgument(argv[++i]) == ParserState::ERROR_STATE) {
				return !CheckForWrongArgumentError(argc, argv, i);
			}
		}
	}
	return !CheckForSyntaxError();
}

void ArgumentParser::DefaultSyntaxExample() {
	PrintDebugMessage(APPLICATION_NAME, "ArgumentParser", "Did not find any command line arguments. Using default values instead.", true);
	PrintDebugMessage(APPLICATION_NAME, "ArgumentParser", "Example of correct command line argument usage:", false);
	PrintDebugMessage(APPLICATION_NAME, "ArgumentParser", "RouteApplication.exe -b 23.7255 37.9666 23.7315 37.9705 -start 0.1 0.1 -end 0.9 0.9", false);
	PrintDebugMessage(APPLICATION_NAME, "ArgumentParser", "Generating a map of Athens, Greece.", false);
	PrintDebugMessage(APPLICATION_NAME, "ArgumentParser", "bounds:\t[-b 23.7255 37.9666 23.7315 37.9705]", false);
	PrintDebugMessage(APPLICATION_NAME, "ArgumentParser", "start:\t[-start 0.1 0.1]", false);
	PrintDebugMessage(APPLICATION_NAME, "ArgumentParser", "end:\t[-end 0.9 0.9]", false);

	bound_query_ = "23.7255,37.9666,23.7315,37.9705";
	starting_point_.x = 0.1;
	starting_point_.y = 0.1;
	ending_point_.x = 0.9;
	ending_point_.y = 0.9;
	syntax_state_ = (int)SyntaxFlags::BOUNDS;
}

bool ArgumentParser::CheckForMissingArgumentError(const int& argc, char** argv, const int i) {
	if (i + number_of_coordinates_to_parse >= argc) {
		cout << "Error parsing arguments: Coordinate argument is missing:" << endl;
		cout << "'";
		for (int j = 0; j < argc; j++) {
			cout << argv[j] << " ";
		}
		cout << "[missing argument]'" << endl;
		return true;
	}
	return false;
}

bool ArgumentParser::CheckForWrongArgumentError(const int& argc, char** argv, const int i) {
		cout << "'";
		for (int j = 0; j < argc; j++) {
			if (argv[j] == argv[i]) {
				cout << "[wrong argument: " << argv[j] << "]";
			}
			else {
				cout << argv[j];
			}
			if (j != argc - 1) {
				cout << " ";
			}
		}
		cout << "'" << endl;
		return true;
}

bool ArgumentParser::CheckForSyntaxError() {
	switch (syntax_state_) {
	case (int)SyntaxFlags::FILE:
		break;
	case (int)SyntaxFlags::BOUNDS:
		break;
	case (int)SyntaxFlags::POINT:
		break;
	case (int)SyntaxFlags::BOUNDS | (int)SyntaxFlags::FILE:
		break;
	case (int)SyntaxFlags::POINT | (int)SyntaxFlags::FILE:
		break;
	default:
		cout << "Error: Incorrect command line argument syntax. Must provide the application with an areas' bounds, a point coordinate or a OSM data file." << endl;
		return true;
	}
	PrintDebugMessage(APPLICATION_NAME, "ArgumentParser", "Syntax analysis completed successfully.", false);
	return false;
}

ArgumentParser::ParserState ArgumentParser::ParseArgument(string_view arg) {
	UpdateInputState(arg);
	UpdateParserState(arg);
	if (current_parser_state_ = ValidateParsingState(arg); current_parser_state_ != ParserState::ERROR_STATE) {
		StoreData(arg);
		ResetParsingState();
	}
	return current_parser_state_;
}

void ArgumentParser::UpdateInputState(string_view arg) {
	InputState input_state_before_update = current_input_state_;
	if (arg == "-b") {
		current_input_state_ = InputState::BOUNDS_COMMAND;
		syntax_state_ |= (int)SyntaxFlags::BOUNDS;
		number_of_coordinates_to_parse = bound_coordinates_;
	}
	else if (arg == "-f") {
		current_input_state_ = InputState::FILE_COMMAND;
		syntax_state_ |= (int)SyntaxFlags::FILE;
	}
	else if (arg == "-p") {
		current_input_state_ = InputState::POINT_COMMAND;
		syntax_state_ |= (int)SyntaxFlags::POINT;
		number_of_coordinates_to_parse = point_coordinates_;
	}
	else if (arg == "-start") {
		current_input_state_ = InputState::START_POINT_COMMAND;
		number_of_coordinates_to_parse = point_coordinates_;
	}
	else if (arg == "-end") {
		current_input_state_ = InputState::END_POINT_COMMAND;
		number_of_coordinates_to_parse = point_coordinates_;
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
	if (input_state_before_update != current_input_state_) {
		previous_input_state_ = input_state_before_update;
	}
}

void ArgumentParser::UpdateParserState(string_view arg) {
	ParserState parser_state_before_update = current_parser_state_;
	current_parser_state_ = static_cast<ParserState>(stateTable_->GetState(current_parser_state_, current_input_state_));
	if (parser_state_before_update != current_parser_state_) {
		previous_parser_state_ = parser_state_before_update;
	}
}

ArgumentParser::ParserState ArgumentParser::ValidateParsingState(string_view arg) {
	switch (current_parser_state_) {
	case ParserState::PARSING_STATE:
		if (number_of_coordinates_to_parse >= 1 && current_input_state_ == InputState::FILENAME) {
			cout << "Error parsing arguments: was expecting a coordinate as an argument. Instead, parsed '" << arg << "'." << endl;
			return ParserState::ERROR_STATE;
		}
		break;
	case ParserState::ERROR_STATE:
		switch (current_input_state_) {
		case InputState::COORDINATE:
			cout << "Error parsing arguments: was not expecting a coordinate as an argument." << endl;
			break;
		default:
			cout << "Error parsing arguments: was not expecting a string as an argument." << endl;
			break;
		}
		break;
	}
	return current_parser_state_;
}

void ArgumentParser::StoreData(string_view arg) {
	switch (previous_input_state_) {
	case InputState::BOUNDS_COMMAND:
		number_of_coordinates_to_parse--;
		if (number_of_coordinates_to_parse == 0) {
			InitializeBounds();
		}
		break;
	case InputState::FILE_COMMAND:
		filename_ = arg;
		break;
	case InputState::START_POINT_COMMAND:
		number_of_coordinates_to_parse--;
		if (number_of_coordinates_to_parse == 0) {
			InitializePoint(starting_point_);
		}
		break;
	case InputState::END_POINT_COMMAND:
		number_of_coordinates_to_parse--;
		if (number_of_coordinates_to_parse == 0) {
			InitializePoint(ending_point_);
		}
		break;
	case InputState::POINT_COMMAND:
		number_of_coordinates_to_parse--;
		if (number_of_coordinates_to_parse == 0) {
			InitializePoint(point_);
		}
		break;
	default:
		break;
	}
}

void ArgumentParser::ResetParsingState() {
	if (number_of_coordinates_to_parse == 0 || previous_input_state_ == InputState::FILE_COMMAND) {
		Reset();
	}
}

void inline ArgumentParser::InitializePoint(Model::Node& node) {
	node.x = coords_[0];
	node.y = coords_[1];
}

void inline ArgumentParser::InitializeBounds() {
	for (int i = 0; i < 3; i++) {
		bound_query_ += to_string(coords_[i]) + ",";
	}
	bound_query_ += to_string(coords_[3]);
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

void ArgumentParser::Release() {
	PrintDebugMessage(APPLICATION_NAME, "ArgumentParser", "Releasing parser resources...", false);
	delete stateTable_;
}

ArgumentParser::~ArgumentParser() {
	Release();
}