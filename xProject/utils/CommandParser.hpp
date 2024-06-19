#pragma once

#include "xProject_pch.hpp"

#include <collections/QeueuLockfree.hpp>

namespace Utils
{
	struct Command
	{
		std::string fullCommand;
		std::string titleCommand;
		std::vector<std::string> arguments;

		explicit Command() = default;
		explicit Command(const std::string& _input) : fullCommand(_input)
		{
			ParsingCommandLine();
		}

	private:
		void ParsingCommandLine()
		{
			std::istringstream iss(fullCommand);
			iss >> titleCommand;

			std::string arg;
			while (iss >> arg)
			{
				arguments.push_back(arg);
			}
		}
	};


	class CommandTask
	{
	private:
		std::function<void(const Command&)> task;
	public:
		template<typename Func, typename... Args>
		CommandTask(Func&& _func, Args&&... _args) 
			: task([func = std::forward<Func>(_func),
					args = std::tuple(std::forward<Args>(_args)...)](const Command& _command)
					{
						std::apply(func, std::tuple_cat(std::make_tuple(_command), args));
					})
		{}

		void Execute(const Command& _command) const { task(_command); }
	
		CommandTask(CommandTask&&) = default;
		CommandTask& operator=(CommandTask&&) = default;
		CommandTask(const CommandTask&) = delete;
		CommandTask& operator=(const CommandTask&) = delete;
	};

	template<typename Func, typename... Args>
	std::unique_ptr<CommandTask> MakeCommandTask(Func&& _func, Args&&... _args)
	{
		return std::make_unique<CommandTask>(std::forward<Func>(_func),
										     std::forward<Args>(_args)...);
	}

	class CommandParser
	{
	private:
		enum class ParserStates
		{
			RUNNING,
			STOPPED
		};

		std::unordered_map<std::string, std::unique_ptr<CommandTask>> callbackCommand;
		std::atomic<ParserStates> parserState = ParserStates::STOPPED;

		Utils::QueueLF<Command> commandQueue;

	public:
		CommandParser() = default;
		~CommandParser()
		{
			StopParse();
		}

		template<typename Func, typename... Args>
		void RegisterCommand(const std::string& _command, Func&& _func, Args&&... _args)
		{
			callbackCommand[_command] = MakeCommandTask(std::forward<Func>(_func), std::forward<Args>(_args)...);
		}

		void ExecuteParse()
		{
			parserState = ParserStates::RUNNING;
			GettingCommandLine();
		}

		void StopParse()
		{
			parserState = ParserStates::STOPPED;
		}

		Utils::QueueLF<Command>& Incoming()
		{
			return commandQueue;
		}

	private:
		void GettingCommandLine()
		{
			while (parserState == ParserStates::RUNNING)
			{
				std::cout << "\033[1;32m>>>\033[0m";

				std::string input;
				std::getline(std::cin, input);
	
				Command inputCommand(input);
				if (callbackCommand.find(inputCommand.titleCommand) != callbackCommand.end())
				{
					callbackCommand[inputCommand.titleCommand]->Execute(inputCommand);
				}
			}
		}
	};
}