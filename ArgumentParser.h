#pragma once
#ifndef ROUTE_APP_ARGUMENT_PARSER_H
#define ROUTE_APP_ARGUMENT_PARSER_H

#include <iostream>

namespace route_app {
	class ArgumentParser {
	public:
		enum class ParserState {
			PARSING_STATE = -3, OK_STATE = -2, ERROR_STATE = -1, START_STATE, BOUNDS_STATE, POINT_STATE, FILE_STATE
		};
		enum class InputState {
			INVALID = -1, BOUNDS_COMMAND, FILE_COMMAND, START_POINT_COMMAND, END_POINT_COMMAND, POINT_COMMAND, COORDINATE, FILENAME
		};
		ArgumentParser();
		~ArgumentParser();
		ParserState ParseArgument(std::string_view arg);
		std::string GetQuery();
	private:
		class StateTable {
		private:
			int* array;
			const size_t width_ = 4;
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
		const int bound_coordinates_ = 4;
		const int point_coordinates_ = 2;
		ParserState currentParserState_;
		ParserState previousParserState_;
		InputState currentInputState_;
		InputState previousInputState_;
		std::string query_;
		void Initialize();
		void CreateStateTable();
		void UpdateInputState(std::string_view arg);
		void UpdateParserState(std::string_view arg);
		void UpdatePreviousStates();
		void Release();
	public:
		StateTable& GetStateTable() const { return *stateTable_; }
	};
}

#endif