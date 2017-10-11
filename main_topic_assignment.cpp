#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <iomanip>

std::vector<std::pair<std::string, std::size_t>> read_in_topics(std::istream& istr)
{
	std::vector<std::pair<std::string, std::size_t>> topics;
	std::string topic_line;
	while(std::getline(istr, topic_line))
	{
		if(topic_line.empty())
			continue;

		topics.push_back({ topic_line, 1 });
	}

	return topics;
}


std::vector<std::pair<std::string, std::vector<unsigned>>> read_in_student_preferences(std::istream& istr)
{
	// read in student preferences
	std::map<std::string, std::vector<unsigned>> student_preferences;
	std::string line;
	std::size_t i_line = 1;
	while(std::getline(istr, line))
	{
		std::stringstream ss(line);
		std::string student_name;
		std::vector<unsigned> student_topic_preferences;
		ss >> student_name;	
		unsigned preference;
		while(!ss.eof())
		{
			ss >> preference;
			if(preference == 0)
			{
				throw std::logic_error("Parsing error in student preference file at line " +  std::to_string(i_line));
			}
			student_topic_preferences.push_back(preference);
		}

		student_preferences.insert({ student_name, student_topic_preferences });

		i_line++;
	}

	return std::vector<std::pair<std::string, std::vector<unsigned>>>(student_preferences.begin(), student_preferences.end());
}

std::vector<unsigned> read_in_weights(std::istream& istr)
{
	std::vector<unsigned> weights;
	while(!istr.eof())
	{
		unsigned weight;	
		istr >> weight;
		weights.push_back(weight);
	}

	return weights;
}

void display_help(std::ostream& out)
{
	constexpr std::size_t width = 30;

	out << "USAGE: ./topic_assignment [topic_file] [student_preference_file] [preference_values_file]" << std::endl;
	out << std::left << std::setw(width) << "[topic_file]:" << "File in which each line contains the name of a topic" << std::endl;
	out << std::left << std::setw(width) << "[student_preference_file]:" << "File in which each line contains a student. Lines start with a name of the student (without spaces) followed by the topic ids (start at 1 for the first topic) ordered by preference.";
	out << " { Example: Benjamin 3 2 4 (means that student Benjamin prefers to have topic 3 over 2 over 4) }" << std::endl;
	out << std::left << std::setw(width) << "[preference_values_file]:" << "File in which line n contains one single number specifying how much weight is put on the weight is put on n-th choice of a student" << std::endl;
}

int main(int argc, char *argv[])
{
	// check if help was requested
	if(argc == 2 && (std::string(argv[1]) == "help" || std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help"))
	{
		display_help(std::cout);
		return 0;
	}

	// check if all parameters were specified
	if(argc != 4)
	{
		std::cerr << "Please provide all needed parameters." << std::endl;
		display_help(std::cerr);
		return 1;
	}

	// open files
	std::ifstream topics_file(argv[1]), student_preferences_file(argv[2]), weight_file(argv[3]);

	// read in data
	auto topics = read_in_topics(topics_file);
	auto student_preferences = read_in_student_preferences(student_preferences_file);
	auto weights = read_in_weights(weight_file);

	std::size_t n_students = student_preferences.size();
	std::size_t n_distinct_topics = topics.size();
	std::size_t n_topics = 0;
	for(auto topic : topics)
		n_topics += topic.second;

	if(n_students != n_topics)
		throw std::logic_error("Number of all topics (with potential duplicates) does not match number of students.");

	// build cost matrix
	std::vector<std::vector<unsigned>> c_matrix(student_preferences.size(), std::vector<unsigned>(n_distinct_topics, 0));
	std::size_t i_student = 0;
	for(auto student_preference : student_preferences)
	{
		for(std::size_t i_pref = 0; i_pref < student_preference.second.size(); i_pref++)
		{
			c_matrix[i_student][student_preference.second[i_pref]-1] = weights[i_pref];
		}

		i_student++;
	}

	// build lp file
	std::ofstream topic_assignment_lp_file("topic_assignment.lp");
	topic_assignment_lp_file << "/* Generated by ./topic_assignment */" << std::endl << std::endl;

	// build target function
	topic_assignment_lp_file << "max: ";
	bool first = true;
	for(std::size_t i_student = 0; i_student < n_students; i_student++)
		for(std::size_t i_topic = 0; i_topic < n_distinct_topics; i_topic++)
		{
			if(c_matrix[i_student][i_topic] == 0)
				continue;

			if(first)
				first = false;
			else 
				topic_assignment_lp_file << " + ";

			topic_assignment_lp_file << c_matrix[i_student][i_topic] << " " << "x_" << i_student << "_" << i_topic;
		}
	topic_assignment_lp_file << ";" << std::endl << std::endl;

	// build constraint that each topic is only picked by one student
	topic_assignment_lp_file << "/* constraint that each topic is only picked by one student */" << std::endl << std::endl;
	for(std::size_t i_topic = 0; i_topic < n_distinct_topics; i_topic++)
	{
		first = true;
		for(std::size_t i_student = 0; i_student < n_students; i_student++)
		{
			if(first)
				first = false;
			else 
				topic_assignment_lp_file << " + ";

			topic_assignment_lp_file <<  "x_" << i_student << "_" << i_topic;
		}
		topic_assignment_lp_file <<  " = 1;" << std::endl;
	}
	topic_assignment_lp_file << std::endl;

	// build constraint that each student only picks one topic
	topic_assignment_lp_file << "/* build constraint that each student only picks one topic */" << std::endl << std::endl;
	for(std::size_t i_student = 0; i_student < n_students; i_student++)
	{
		first = true;
		for(std::size_t i_topic = 0; i_topic < n_distinct_topics; i_topic++)
		{
			if(first)
				first = false;
			else 
				topic_assignment_lp_file << " + ";

			topic_assignment_lp_file <<  "x_" << i_student << "_" << i_topic;
		}
		topic_assignment_lp_file <<  " = 1;" << std::endl;
	}
	topic_assignment_lp_file << std::endl;

	for(std::size_t i_topic = 0; i_topic < n_distinct_topics; i_topic++)
		for(std::size_t i_student = 0; i_student < n_students; i_student++)
			topic_assignment_lp_file <<  "bin x_" << i_student << "_" << i_topic << ";" << std::endl;


	// run lp_solve on problem instance and save output to file
	std::system("lp_solve topic_assignment.lp | tail -n +5 > assignment_out.txt");

	// open output file and read it
	std::ifstream result_file("assignment_out.txt");
	std::vector<std::vector<unsigned>> result_matrix(n_students, std::vector<unsigned>(n_distinct_topics, 0));
	std::string line;
	while(std::getline(result_file, line))
	{
		std::stringstream ss(line);
		std::string variable, value;
		ss >> variable >> value;

		std::stringstream ss2(variable);
		std::string i_student_str, i_topic_str, dump;
		std::getline(ss2, dump, '_');
		std::getline(ss2, i_student_str, '_');
		std::getline(ss2, i_topic_str, '_');

		std::size_t i_student = std::stoul(i_student_str);
		std::size_t i_topic = std::stoul(i_topic_str);

		if(value == "0")
			result_matrix[i_student][i_topic] = 0;
		else
			result_matrix[i_student][i_topic] = 1;
	}

	// check consistency of result_matrix
	for(std::size_t i_student = 0; i_student < n_students; i_student++)
	{
		std::size_t n_trues = 0;
		for(std::size_t i_topic = 0; i_topic < n_distinct_topics; i_topic++)
			n_trues += (result_matrix[i_student][i_topic]) ? 1 : 0;
		if(n_trues != 1)
		{
			std::cerr << "Error" << std::endl;
		}
	}

	for(std::size_t i_topic = 0; i_topic < n_distinct_topics; i_topic++)
	{
		std::size_t n_trues = 0;
		for(std::size_t i_student = 0; i_student < n_students; i_student++)
			n_trues += (result_matrix[i_student][i_topic]) ? 1 : 0;
		if(n_trues != 1)
		{
			std::cerr << "Error" << std::endl;
			return 1;
		}
	}

	for(std::size_t i_student = 0; i_student < n_students; i_student++)
		for(std::size_t i_topic = 0; i_topic < n_distinct_topics; i_topic++)
			if(result_matrix[i_student][i_topic])
			{
				std::cout <<  student_preferences[i_student].first << " get the topic " << topics[i_topic].first << std::endl;
				break;
			}

	std::remove("topic_assignment.lp");
	std::remove("assignment_out.txt");

	return 0;
}
