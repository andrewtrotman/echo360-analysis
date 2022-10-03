/*
	ECHO360-ANALYSIS.CPP
	--------------------
	gnuplot commands to draw the output of this program (plot.gpl):
		set datafile separator ","
		set boxwidth 0.75 relative
		set xtics rotate by 90 right
		set yrange [0:100]
		set style fill solid border
		set title whichdata
		set ylabel "Percent of Class"
		set key off
		set term pdf
		set term pdfcairo size 11.7in,8.3in
		set output whichdata.".pdf"
		plot whichdata.".dat" using 4:xtic(1) with boxes linecolor rgb "#000080"
	Execute this with:
		./echo360-analysis 10 "../COSC431  student sem1.csv" > cosc431.dat
		gnuplot -e "whichdata='cosc431'" plot.gpl
*/
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

/*
	CLASS LECTURE_STATISTICS
	------------------------
*/
class lecture_statistics
	{
	public:
		std::string class_name;
		std::string class_date;
		size_t students_who_watched;
		size_t percent_students_who_watched;
		size_t class_size;

	public:
		lecture_statistics(const std::string &class_name, const std::string &class_date, size_t students_who_watched, size_t class_size) :
			class_name(class_name),
			class_date(class_date),
			students_who_watched(students_who_watched),
			class_size(class_size),
			percent_students_who_watched((size_t)((double)students_who_watched / (double) class_size * 100))
			{
			/* Nothing */
			}

		/*
			OPERATOR<()
			-----------
		*/
		bool operator<(const lecture_statistics &that) const
			{
			return students_who_watched > that.students_who_watched;
			}
	};

	/*
		OPERATOR<<()
		-----------
	*/
	std::ostream &operator<<(std::ostream &out, const lecture_statistics &object)
		{
		if (object.class_date == "N/A")
			out << '"' << object.class_name << "\"," << object.students_who_watched << "," << object.class_size << "," << object.percent_students_who_watched << '\n';
		else
			out << "\"L" << object.class_date << "\"," << object.students_who_watched << "," << object.class_size << "," << object.percent_students_who_watched << '\n';

		return out;
		}



/*
	CLASS STUDENT_LECTURE_DETAILS
	-----------------------------
*/
class student_lecture_details
	{
	public:
		std::string student_name;			// student name
		std::string student_email;			// student email address
		std::string class_name;				// lecture name
		std::string class_date;				// the date of the lecture (necessary when the name of two or more lectures is the ssame).
		bool summary;							// is this summary stats for the student? Else its for an individual lecture
		long video_views;						// number of times the video has been viewed
		long video_percent_viewed;			// total proportion of the video that has been viewed

	public:
		/*
			OPERATOR==()
			------------
		*/
		bool operator==(const student_lecture_details &that) const
			{
			return class_name == that.class_name && class_date == that.class_date;
			}

		/*
			OPERATOR<()
			-----------
		*/
		bool operator<(const student_lecture_details &that) const
			{
			if (class_name < that.class_name)
				return true;
			if (class_name > that.class_name)
				return false;

			if (class_date < that.class_date)
				return true;
			if (class_date > that.class_date)
				return false;

			if (student_name < that.student_name)
				return true;

			return false;
			}
	};

/*
	CLASS PARSE
	-----------
*/
class parse
	{
	private:
		const char *data;
	public:
		parse(const char *file) :
			data(file)
			{
			// Nothing
			}

		std::string get(void)
			{
			const char *start = data;
			const char *end;

			while (*start != '"')
				start++;

			end = start + 1;
			while (*end != '"')
				end++;

			data = end + 1;

			return std::string(start + 1, end);
		}
	};

/*
	READ_ENTIRE_FILE()
	------------------
*/
size_t read_entire_file(const std::string &filename, std::string &into)
	{
	FILE *fp;
	struct stat details;
	size_t file_length = 0;

	if ((fp = fopen(filename.c_str(), "rb")) != nullptr)
		{
		if (fstat(fileno(fp), &details) == 0)
			if ((file_length = details.st_size) != 0)
				{
				into.resize(file_length);
				if (fread(&into[0], details.st_size, 1, fp) != 1)
					into.resize(0);
				}
		fclose(fp);
		}

	return file_length;
	}

/*
	BUFFER_TO_LIST()
	----------------
*/
void buffer_to_list(std::vector<char *> &line_list, std::string &buffer)
	{
	char *pos;
	size_t line_count = 0;

	/*
		Walk the buffer counting how many lines we think are in there.
	*/
	pos = &buffer[0];
	while (*pos != '\0')
		{
		if (*pos == '\n' || *pos == '\r')
			{
			/*
				a seperate line is a consequative set of '\n' or '\r' lines.  That is, it removes blank lines from the input file.
			*/
			while (*pos == '\n' || *pos == '\r')
				pos++;
			line_count++;
			}
		else
			pos++;
		}

	/*
		resize the vector to the right size, but first clear it.
	*/
	line_list.clear();
	line_list.reserve(line_count);

	/*
		Now rewalk the buffer turning it into a vector of lines
	*/
	pos = &buffer[0];
	if (*pos != '\n' && *pos != '\r' && *pos != '\0')
		line_list.push_back(pos);
	while (*pos != '\0')
		{
		if (*pos == '\n' || *pos == '\r')
			{
			*pos++ = '\0';
			/*
				a seperate line is a consequative set of '\n' or '\r' lines.  That is, it removes blank lines from the input file.
			*/
			while (*pos == '\n' || *pos == '\r')
				pos++;
			if (*pos != '\0')
				line_list.push_back(pos);
			}
		else
			pos++;
		}
	}

/*
	GET_FIELDS_FROM_LINE()
	----------------------
	Line format:
	"Student Name","Student Email","Student User ID","Section Name","Class","Lesson Date","Total Engagement","Weighted Engagement %","Attendance %","Video Views","Video View %","Total Slides in Deck","Slide Deck Views","Slides Viewed Count","Poll Question Count","Poll Responses Total","Poll Response Correct Count","Poll Response Incorrect Count","Note Events","Q&A Events","Confusion Flags Enabled"
*/
void get_fields_from_line(student_lecture_details &answer, const std::string &line)
	{
	parse parser(line.c_str());

	answer.student_name = parser.get();							// "Student Name"
	answer.student_email = parser.get();						// "Student Email"
	parser.get();														// "Student User ID"
	answer.summary = parser.get() == "" ? false : true;	// "Section Name"
	answer.class_name = parser.get();							// "Class"
	answer.class_date = parser.get();							// "Lesson Date"
	parser.get();														// Total Engagement"
	parser.get();														// "Weighted Engagement %"
	parser.get();														// "Attendance %"
	answer.video_views = atol(parser.get().c_str());		// "Video Views"
	answer.video_percent_viewed = atol(parser.get().c_str()); // "Video View %"
	}

/*
	MAIN()
	------
*/
int main(int argc, const char *argv[])
	{
	if (argc != 3)
		exit(printf("Usage: %s <WatchPercentRequirement> <student.csv>\n", argv[0]));

	long MIN_PERCENT_VIEW_REQUIREMENT = atol(argv[1]);
	const char *filename = argv[2];

	/*
		Block read the data and convert into a list
	*/
	std::string buffer;
	if (read_entire_file(filename, buffer) == 0)
		exit(printf("Failed to read file %s\n", filename));

	std::vector<char *> line_list;
	buffer_to_list(line_list, buffer);

	/*
		Process each line in the input file and build a list of those rows we're interested in
	*/
	std::vector<student_lecture_details> paper_data;
	size_t line_number = 0;
	size_t total_student_count = 0;
	std::string last_student_name = "";
	for (const auto &line : line_list)
		{
		line_number++;

		/*
			Ignore the first line because its the column headings.
		*/
		if (line_number == 1)
			continue;

		/*
			Get the data and shove it into a vector
		*/
		student_lecture_details into;
		get_fields_from_line(into, line);

		if (!into.summary && into.video_views != 0 && into.video_percent_viewed > MIN_PERCENT_VIEW_REQUIREMENT)
			if (into.student_email.find("student") != std::string::npos)
				paper_data.push_back(into);

		/*
			Count the number of students
		*/
		if (into.student_name != last_student_name)
			total_student_count++;
		last_student_name = into.student_name;
		}

	/*
		Sort the data on LectureName + date + StudentName
	*/
	std::sort(paper_data.begin(), paper_data.end());

	/*
		Count and output the number of students for each lecture
	*/
	std::vector<lecture_statistics> true_lecture_statistics;
	size_t count = 1;
	for (size_t row = 1; row < paper_data.size(); row++)
		{
		if (paper_data[row] == paper_data[row - 1])
			count++;
		else
			{
			true_lecture_statistics.push_back(lecture_statistics(paper_data[row - 1].class_name, paper_data[row - 1].class_date, count, total_student_count));
			count = 1;
			}
		}
	if (paper_data.size() != 0)
		true_lecture_statistics.push_back(lecture_statistics(paper_data[paper_data.size() - 1].class_name, paper_data[paper_data.size() - 1].class_date, count, total_student_count));

	/*
		Sort the lecture from most viewed to least viewed
	*/
	std::sort(true_lecture_statistics.begin(), true_lecture_statistics.end());

	for (const auto &current : true_lecture_statistics)
		std::cout << current;

#ifdef NEVER
	/*
		Dump out the list of who watched which lectures
	*/
	std::cout << "\n\n";
	for (const auto &into : paper_data)
		if (into.class_name == "L6 - Hello World")
			std::cout << into.student_name << ',' << into.class_name << '[' << into.class_date << ']' << into.video_views << ' ' << into.video_percent_viewed << '\n';
#endif
	return 0;
	}
