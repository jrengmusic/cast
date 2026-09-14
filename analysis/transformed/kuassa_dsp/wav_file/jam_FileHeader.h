/**
 * @file jam_FileHeader.h
 * @brief RIFF/WAVE chunk header structs and a WavFile reader decoding
 *        16/24/32-bit PCM or 32-bit float sample data to double precision.
 */

namespace jam::dsp
{
/*__________________________________________________________________________________________*/

/** @brief RIFF chunk header: "RIFF" ID, total file size, and "WAVE" format tag. */
struct FileHeader
{
	char chunkID[4];///< RIFF chunk identifier, expected to be "RIFF"
	long chunkSize;///< Size of the file in bytes, minus 8 bytes for chunkID and chunkSize itself
	char format[4];///< Format tag, expected to be "WAVE"
};

/** @brief WAV "fmt " chunk header describing the PCM/float encoding of the sample data. */
struct FormatHeader
{
	char fmtChunkID[4];///< Format chunk identifier, expected to be "fmt "
	int fmtChunkSize;///< Size of the format chunk in bytes
	short audioFormat;///< Audio format tag (1 = PCM, 3 = IEEE float)
	short numChannels;///< Number of interleaved audio channels
	int sampleRate;///< Sample rate in Hz
	int byteRate;///< Average bytes per second
	short blockAlign;///< Bytes per sample frame (all channels)
	short bitsPerSample;///< Bits per sample (16, 24, or 32)
};

/** @brief WAV "data" chunk header preceding the raw sample bytes. */
struct DataHeader
{
	char dataChunkID[4];///< Data chunk identifier, expected to be "data"
	int dataChunkSize;///< Size of the sample data in bytes
};

/**
 * @class WavFile
 * @brief Reads a 16/24/32-bit PCM or 32-bit float WAV file into an internal double-precision sample buffer.
 *
 * Parses the RIFF/fmt/data chunk structure, converts native-endian or
 * byte-swapped (via inherited EndianSwap) sample data of the detected bit
 * depth into normalized doubles, and exposes the decoded sample data,
 * channel count, and sample rate through simple accessors.
 */
class WavFile : public EndianSwap
{

public:

	/**
	 * @brief Constructs a WavFile with no data loaded, detecting the host's endianness.
	 */
	WavFile();		//constructor

	/**
	 * @brief Validates that a file is a readable WAV without loading its sample data.
	 * @param wavFile The path to the WAV file to validate.
	 * @return True if the file parses as a supported RIFF/WAVE file with a readable bit depth, false otherwise.
	 */
	bool testWav (const char* wavFile);

	/**
	 * @brief Opens and fully decodes a WAV file's sample data into the internal buffer.
	 * @param wavfile The path to the WAV file to open.
	 * @return True if the file was successfully opened and decoded, false otherwise.
	 */
	bool openWav (const char* wavfile);

	/**
	 * @brief Releases the decoded sample data buffer, if the file was validated.
	 */
	void closeWav();

	/**
	 * @brief Retrieves a single decoded sample by position.
	 * @param position The sample index to retrieve.
	 * @return The sample value at position, or 0.0 if the file is not validated or position is out of range.
	 */
	double getSample (long position);

	/**
	 * @brief Retrieves a pointer to the decoded sample data buffer.
	 * @return A pointer to the sample data, or nullptr if the file is not validated.
	 */
	double *getSampleData();

	/**
	 * @brief Checks whether a WAV file has been successfully opened and decoded.
	 * @return True if validated, false otherwise.
	 */
	bool isValidated();

	/**
	 * @brief Retrieves the total number of decoded samples (across all channels).
	 * @return The sample count, or 0 if the file is not validated.
	 */
	long getNumSamples();

	/**
	 * @brief Retrieves the number of audio channels.
	 * @return The channel count, or 1 if the file is not validated.
	 */
	short getNumChannel();

	/**
	 * @brief Retrieves the sample rate.
	 * @return The sample rate in Hz, or 0 if the file is not validated.
	 */
	int getSampleRate();

	//void displayWavInfo (void);

private:

	FileHeader fileHeader;		//instances of the three structures for holding Chunk info for File/Format/Data
	FormatHeader fmtHeader;
	DataHeader dataHeader;

	inline double sixteenBitTodouble (short input);
	inline double twentyfourBitTodouble (int input);
	inline double thirtytwoBitTodouble (int input);

	bool validated;

	long numSamples;			//the number of samples, determined during the openwav routine
	double *sampleData;			//pointer to memory to store the actual samples in

	int sampleRate;
};

/**____________________________________END OF NAMESPACE_____________________________________*/
} /** namespace jam::dsp */
