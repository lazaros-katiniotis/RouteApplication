#pragma once
#ifndef ROUTE_APP_ARGUMENT_PARSER_H
#define ROUTE_APP_ARGUMENT_PARSER_H

#include <iostream>
#include "Model.h"

namespace route_app {
	class ArgumentParser {
	public:
		enum class ParserState {
			OK_STATE = -2, ERROR_STATE = -1, START_STATE, BOUNDS_STATE, POINT_STATE, FILE_STATE, PARSING_STATE
		};

		enum class InputState {
			INVALID = -1, BOUNDS_COMMAND, FILE_COMMAND, START_POINT_COMMAND, END_POINT_COMMAND, POINT_COMMAND, COORDINATE, FILENAME
		};

		enum class SyntaxFlags {
			BOUNDS = 1, FILE = 2, POINT = 4
		};

		ArgumentParser(const int &argc, char **argv);
		~ArgumentParser();
		bool SyntaxAnalysis(const int& argc, char** argv);
		std::string GetBoundQuery() const { return bound_query_; }
		std::string GetFilename() const { return filename_; }
		Model::Node GetStartingPoint() const { return starting_point_; }
		Model::Node GetEndingPoint() const { return ending_point_; }
		Model::Node GetPoint() const { return point_; }
		ParserState GetParserState() const { return current_parser_state_; }
		int GetSyntaxState() const { return syntax_state_; }
	private:
		class StateTable {
		private:
			int* array;
			const size_t width_ = 5;
			const size_t height_ = 7;
			size_t Index(int x, int y) const;
		public:
			StateTable() {
				array = new int[width_ * height_];
			}
			~StateTable() {
				delete[] array;
			}
			void SetState(int x, int y, ParserState state) const;
			void SetState(ParserState x, InputState y, ParserState state) const;
			int GetState(int x, int y) const;
			int GetState(ParserState x, InputState y) const;
			size_t GetWidth() const { return width_; }
			size_t GetHeight() const { return height_; }
		};
		StateTable* stateTable_;
		ParserState current_parser_state_;
		ParserState previous_parser_state_;
		InputState current_input_state_;
		InputState previous_input_state_;
		int number_of_coordinates_to_parse;
		const int bound_coordinates_ = 4;
		const int point_coordinates_ = 2;
		unsigned int syntax_state_;
		vector<double> coords_;

		Model::Node point_;
		Model::Node starting_point_;
		Model::Node ending_point_;
		std::string bound_query_;
		std::string filename_;
		void Initialize(const int& argc, char** argv);
		ParserState ParseArgument(std::string_view arg);
		void CreateStateTable();
		void UpdateInputState(std::string_view arg);
		void UpdateParserState(std::string_view arg);
		ParserState ValidateParsingState(std::string_view arg);
		void ResetParsingState();
		void Reset();
		void StoreData(std::string_view arg);
		void InitializePoint(Model::Node& node);
		void InitializeBounds();
		bool CheckForMissingArgumentError(const int& argc, char** argv, const int i);
		bool CheckForWrongArgumentError(const int& argc, char** argv, const int i);
		bool CheckForSyntaxError();
		void Release();
	public:
		StateTable& GetStateTable() const { return *stateTable_; }
	};
}

#endif