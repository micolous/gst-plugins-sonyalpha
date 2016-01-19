import struct, argparse, os

def extract_frames(input_f, output_fstr):
	while input_f:
		# keep reading the file until the end
		# read the common header
		common_header = input_f.read(8)
		assert len(common_header) == 8
		
		start_byte, payload_type, sequence_no, timestamp = struct.unpack('!BBHI', common_header)
		assert start_byte == 0xFF, 'start byte is %x' % start_byte
		
		# read the common header
		payload_header = input_f.read(8)
		assert payload_header[:4] == '\x24\x35\x68\x79'
		payload_size1, payload_size2, padding_size = struct.unpack('!BHB', payload_header[4:])
		
		payload_size = (payload_size1 << 16) + payload_size2

		if payload_type == 0x01:
			input_f.seek(4, os.SEEK_CUR)
			assert input_f.read(1) == '\0'
			input_f.seek(115, os.SEEK_CUR)
			
			# read jpeg data
			assert not os.path.exists(output_fstr % timestamp)
			print '.',
			output_f = open(output_fstr % timestamp, 'wb')
			output_f.write(input_f.read(payload_size))
			output_f.close()
			
			if padding_size:
				input_f.seek(padding_size, os.SEEK_CUR)
		elif payload_type == 0x02:
			frame_info = input_f.read(6)
			frame_info_version, frame_count, frame_info_size = struct.unpack('!HHH', frame_info)
			input_f.seek(114, os.SEEK_CUR)
			
			# ignore extra info
			input_f.seek(frame_info_size + padding_size, os.SEEK_CUR)

def main():
	parser = argparse.ArgumentParser()
	parser.add_argument('input', type=argparse.FileType('rb'), nargs=1)
	parser.add_argument('-o', '--output', default='frame-%010d.jpg')
	
	options = parser.parse_args()
	extract_frames(options.input[0], options.output)

if __name__ == '__main__':
	main()

