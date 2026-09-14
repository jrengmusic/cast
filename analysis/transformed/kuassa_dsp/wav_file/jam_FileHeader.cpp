namespace jam::dsp
{
/*__________________________________________________________________________________________*/

bool BigEndianSystem;  //you might want to extern this

void InitEndian()
{
	unsigned char SwapTest[2] = { 1, 0 };

	if( *(short *) SwapTest == 1 )
	{
		//little endian
		BigEndianSystem = false;
	}
	else
	{
		//big endian
		BigEndianSystem = true;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WavFile::WavFile()		//constructor
{
	sampleRate = 0;
	sampleData = 0;
	numSamples = 0;
	validated = false;

	InitEndian();
}

bool WavFile::testWav (const char* wavfile)
{
	bool BigEndianSystem_ = BigEndianSystem;

	char* buffer;		//buffer for initial reading of data from file

	buffer = 0;

	numSamples = 0;
	validated = false;

    std::ifstream infile;			//creates an instance of the input stream class

    infile.open (wavfile, std::ios::binary);

	if (infile.good() != 1)
	{
		infile.close();
		return false;
	}

    infile.seekg (0, std::ios::end);									//send cursor to the end of the file
	int length = (int)infile.tellg();								//set the length to the current location
    infile.seekg (0, std::ios::beg);									//send cursor back to the beginning of the file

	if (length == 0)
	{
		infile.close();
		return false;
	}

	buffer = new char[length];

	infile.read((char* )buffer, 4);
	if (strncmp((char* )buffer, "RIFF", 4) != 0)
	{
		infile.close();
		if (buffer)
			delete[] buffer;

		return false;
	}

	infile.seekg (8);
	infile.read((char* )buffer, 4);
	if (strncmp((char* )buffer, "WAVE", 4) != 0)
	{
		infile.close();
		if (buffer)
			delete[] buffer;

		return false;
	}

    infile.seekg (0, std::ios::beg);

	int i = 0;
	bool bExit = false;
	int validCheck = 0;
	int fmtStart = 0;
	int dataStart = 0;

	while (bExit == false)
	{
		infile.read((char* )buffer, 4);

		if (strncmp((char* )buffer, "fmt ", 4) == 0)
		{
			fmtStart = i;
			validCheck++;
		}

		if (strncmp((char* )buffer, "data", 4) == 0)
		{
			dataStart = i;
			validCheck++;
		}

		if (infile.eof())
		{
			bExit = true;
		}

		if (validCheck == 2)
		{
			bExit = true;
		}

		i++;
		infile.seekg (i);
	}

	//VALIDATE THE FILE
	if (validCheck != 2)
	{
		infile.close();
		if (buffer)
			delete[] buffer;

		return false;
	}

	//READ INFO FROM THE RIFF CHUNK
	infile.seekg (0);										//jump to offset from beginning of file
	infile.read((char* )&fileHeader, sizeof (fileHeader));	//read into the CFormatHeader instance

	//READ INFO FROM THE FORMAT CHUNK
	infile.seekg (fmtStart);									//jump to offset from beginning of file
	infile.read((char* )&fmtHeader, sizeof (fmtHeader));		//read into the CFormatHeader instance

	if (BigEndianSystem_)
	{
		if (shortswap (fmtHeader.audioFormat) == 1)
		{
		}
		else if (shortswap (fmtHeader.audioFormat) == 3)
		{
		}
		else
		{
			infile.close();
			if (buffer)
				delete[] buffer;
			return false;
		}
	}
	else
	{
		if (fmtHeader.audioFormat == 1)
		{
		}
		else if (fmtHeader.audioFormat == 3)
		{
		}
		else
		{
			infile.close();
			if (buffer)
				delete[] buffer;
			return false;
		}
	}

	//READ INFO FROM THE DATA CHUNK
	infile.seekg (dataStart);
	infile.read((char* )&dataHeader, sizeof (dataHeader));

	//READ THE ACTUAL SAMPLE DATA FROM THE DATA CHUNK
	infile.seekg (dataStart + 8);

	if (BigEndianSystem_)
		length = (intswap (dataHeader.dataChunkSize) / shortswap (fmtHeader.blockAlign) * shortswap (fmtHeader.numChannels));
	else
		length = (dataHeader.dataChunkSize / fmtHeader.blockAlign * fmtHeader.numChannels);

	numSamples = length;

	if (length == 0)
	{
		infile.close();
		validated = false;
		if (buffer)
			delete[] buffer;
		return false;
	}

	short bitsPerSample_;

	if (BigEndianSystem_)
		bitsPerSample_ = shortswap (fmtHeader.bitsPerSample);
	else
		bitsPerSample_ = fmtHeader.bitsPerSample;

	switch (bitsPerSample_)
	{
	case 16:
		break;
	case 24:
		break;
	case 32:
		break;
	default:
		infile.close();
		if (buffer)
			delete[] buffer;
		return false;
		break;
	}

	infile.close();

	if (buffer)
		delete[] buffer;

	return true;
}

bool WavFile::openWav (const char* wavfile)
{
	bool BigEndianSystem_ = BigEndianSystem;

	char* buffer;		//buffer for initial reading of data from file

	buffer = 0;

	sampleData = 0;
	numSamples = 0;
	validated = false;

    std::ifstream infile;			//creates an instance of the input stream class
	bool isFloat = false;

    infile.open (wavfile, std::ios::binary);

	if (infile.good() != 1)
	{
		infile.close();
		validated = false;
		return false;
	}

    infile.seekg (0, std::ios::end);									//send cursor to the end of the file
	int length = (int)infile.tellg();								//set the length to the current location
    infile.seekg (0, std::ios::beg);									//send cursor back to the beginning of the file

	if (length == 0)
	{
		infile.close();
		validated = false;
		return false;
	}

	buffer = new char [length];

	infile.read((char* )buffer, 4);
	if (strncmp((char* )buffer, "RIFF", 4) != 0)
	{
		infile.close();
		validated = false;
		if (buffer)
			delete [] buffer;

		return false;
	}

	infile.seekg (8);
	infile.read((char* )buffer, 4);
	if (strncmp((char* )buffer, "WAVE", 4) != 0)
	{
		infile.close();
		validated = false;
		if (buffer)
			delete [] buffer;

		return false;
	}

    infile.seekg (0, std::ios::beg);

	int i = 0;
	bool bExit = false;
	int validCheck = 0;
	int fmtStart = 0;
	int dataStart = 0;

	while (bExit == false)
	{
		infile.read((char* )buffer, 4);

		if (strncmp((char* )buffer, "fmt ", 4) == 0)
		{
			fmtStart = i;
			validCheck++;
		}

		if (strncmp((char* )buffer, "data", 4) == 0)
		{
			dataStart = i;
			validCheck++;
		}

		if (infile.eof())
		{
			bExit = true;
		}

		if (validCheck == 2)
		{
			bExit = true;
		}

		i++;
		infile.seekg (i);
	}

	//VALIDATE THE FILE
	if (validCheck != 2)
	{
		infile.close();
		validated = false;
		if (buffer)
			delete [] buffer;

		return false;
	}

	//READ INFO FROM THE RIFF CHUNK
	infile.seekg (0);										//jump to offset from beginning of file
	infile.read((char* )&fileHeader, sizeof (fileHeader));	//read into the CFormatHeader instance

	//READ INFO FROM THE FORMAT CHUNK
	infile.seekg (fmtStart);									//jump to offset from beginning of file
	infile.read((char* )&fmtHeader, sizeof (fmtHeader));		//read into the CFormatHeader instance

	if (BigEndianSystem_)
	{
		if (shortswap (fmtHeader.audioFormat) == 1)
		{
			isFloat = false;
		}
		else if (shortswap (fmtHeader.audioFormat) == 3)
		{
			isFloat = true;
		}
		else
		{
			isFloat = false;
			infile.close();
			validated = false;
			if (buffer)
				delete [] buffer;
			return false;
		}
	}
	else
	{
		if (fmtHeader.audioFormat == 1)
		{
			isFloat = false;
		}
		else if (fmtHeader.audioFormat == 3)
		{
			isFloat = true;
		}
		else
		{
			isFloat = false;
			infile.close();
			validated = false;
			if (buffer)
				delete [] buffer;
			return false;
		}
	}

	sampleRate = fmtHeader.sampleRate;

	//READ INFO FROM THE DATA CHUNK
	infile.seekg (dataStart);
	infile.read((char* )&dataHeader, sizeof (dataHeader));

	//READ THE ACTUAL SAMPLE DATA FROM THE DATA CHUNK
	infile.seekg (dataStart + 8);

	if (BigEndianSystem_)
	{
		length = (intswap (dataHeader.dataChunkSize) / shortswap (fmtHeader.blockAlign) * shortswap (fmtHeader.numChannels));
	}
	else
	{
		length = (dataHeader.dataChunkSize / fmtHeader.blockAlign * fmtHeader.numChannels);
	}

	numSamples = length;

	if (length == 0)
	{
		infile.close();
		validated = false;
		if (buffer)
			delete [] buffer;
		return false;
	}

	sampleData = new double[length];	//creates a new area of memory for the sample block
	for (i = 0; i < length; i++)
	{
		sampleData[i] = 0;
	}

	short bitsPerSample_;

	if (BigEndianSystem_)
	{
		bitsPerSample_ = shortswap (fmtHeader.bitsPerSample);
	}
	else
	{
		bitsPerSample_ = fmtHeader.bitsPerSample;
	}

	switch (bitsPerSample_)
	{
	case 16:
		{
			union uni16			//union allows access to the same data in multiple forms
			{
				short s;		//here: as either a TWO BYTE short, referenced using dot operator & s
				struct bytes
				{
					char b0;	//or:   as TWO, SINGLE BYTE chars, referenced using dot operator & b
					char b1;
				}b;
			}convertion_union16;

			for (i = 0; i < length; i++)		//loop for every sample
			{
				// read into short
				infile.read((char* )&convertion_union16.b.b0,1);	//read ONE byte into the b0 variable
				infile.read((char* )&convertion_union16.b.b1,1);	//read ONE byte into the b1 variable

				// convert to 1.0
				short tempvalue = convertion_union16.s;

				if (BigEndianSystem_)
				{
					tempvalue = shortswap (tempvalue);
				}

				sampleData[i] = sixteenBitTodouble (tempvalue);		//read the b0 & b1 variables as a single SHORT
			}
			break;
		}

	case 24:
		{
			union uni24
			{
				int s;
				struct bytes
				{
					char b0;
					char b1;
					char b2;
					char b3;
				}b;
			}convertion_union24;

			for (i = 0; i < length; i++)
			{
				infile.read((char* )&convertion_union24.b.b0,1);
				infile.read((char* )&convertion_union24.b.b1,1);
				infile.read((char* )&convertion_union24.b.b2,1);
				convertion_union24.b.b3=0;

				int tempvalue = convertion_union24.s;

				if (BigEndianSystem_)
				{
					tempvalue = intswap (tempvalue);
				}

				sampleData[i] = twentyfourBitTodouble (tempvalue);

				//added by Aradaz for 24bit
				sampleData[i] = sampleData[i] > 1.0 ? - 2.0 + sampleData[i] : sampleData[i];
			}

			break;
		}

	case 32:
		{
			union uni32
			{
				int s;
				float sf;
				struct bytes
				{
					char b0;
					char b1;
					char b2;
					char b3;
				}b;
			}convertion_union32;

			for (i = 0; i < length; i++)
			{
				infile.read((char* )&convertion_union32.b.b0,1);
				infile.read((char* )&convertion_union32.b.b1,1);
				infile.read((char* )&convertion_union32.b.b2,1);
				infile.read((char* )&convertion_union32.b.b3,1);

				if (isFloat)
				{
					float tempvalue = convertion_union32.sf;

					if (BigEndianSystem_)
					{
						tempvalue = floatswap (tempvalue);
					}

					sampleData[i] = (double)tempvalue;
				}
				else
				{
					int tempvalue = convertion_union32.s;

					if (BigEndianSystem_)
					{
						tempvalue = intswap (tempvalue);
					}

					sampleData[i] = thirtytwoBitTodouble (tempvalue);
				}
			}
			break;
		}

	default:
		{
			//cout<<  "Unsupported Wav Format"<<endl;
			infile.close();
			validated = false;
			if (buffer)
				delete [] buffer;
			if (sampleData)
				delete [] sampleData;
			return false;
			break;
		}
	}//end switch

	infile.close();
	validated = true;

	if (buffer)
		delete [] buffer;

	return true;
}

void WavFile::closeWav()
{
	if (isValidated())
	{
		if (sampleData)
			delete [] sampleData;
	}
}

bool WavFile::isValidated()
{
	return validated;
}

double WavFile::getSample (long position)
{
	double sample = 0.0;

	if (isValidated())
	{
		if (position < numSamples && position >= 0)
		{
			sample = sampleData[position];
		}
	}

	return sample;
}

double* WavFile::getSampleData()
{
	if (isValidated())
	{
		return sampleData;
	}
	else
	{
		return 0;
	}
}

long WavFile::getNumSamples()
{
	if (isValidated())
	{
		return numSamples;
	}
	else
	{
		return 0;
	}
}

short WavFile::getNumChannel()
{
	if (isValidated())
	{
		if (BigEndianSystem)
		{
			return shortswap (fmtHeader.numChannels);
		}
		else
		{
			return fmtHeader.numChannels;
		}
	}
	else
	{
		return 1;
	}
}

int WavFile::getSampleRate()
{
	if (isValidated())
		return sampleRate;
	else
		return 0;
}

inline double WavFile::sixteenBitTodouble (short input)
{
	double max = 32767.0;
	double min = 32768.0;
	double sample = 0.0;

	if (input > 0)
	{
		sample = (double)input/max;
	}
	else
	{
		sample = (double)input/min;
	}

	return sample;
}

inline double WavFile::twentyfourBitTodouble (int input)
{
	double max = 8388607.0;
	double min = 8388608.0;
	double sample = 0.0;

	if (input > 0)
	{
		sample = (double)input/max;
	}
	else
	{
		sample = (double)input/min;
	}

	return sample;
}

inline double WavFile::thirtytwoBitTodouble (int input)
{
	double max = 2147483647.0;
	double min = 2147483648.0;
	double sample = 0.0;

	if (input > 0)
	{
		sample = (double)input/max;
	}
	else
	{
		sample = (double)input/min;
	}

	return sample;
}

/**____________________________________END OF NAMESPACE_____________________________________*/
} /** namespace jam::dsp */
