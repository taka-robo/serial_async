#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/array.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <fstream>

using namespace boost::asio;
using namespace boost::posix_time;

typedef struct _Force{
	double rad_xy;
	double Fx;
	double Fy;
	double Fz;
	double Mx;
	double My;
	double Mz;
};

static bool isRunning(true);
const char *PORT = "/dev/ttyUSB0";
io_service io;
serial_port port( io, PORT );
boost::array<char, 64> rbuf;
int fp = 0;
char fbuf[128] = {0};
std::ofstream ofs;
boost::mutex mtx;
_Force f42, f45;

static void waitKeyPressed(void)
{
	getchar();
	isRunning = false;
}

void read_callback(const boost::system::error_code& e, std::size_t size)
{
	boost::mutex::scoped_lock lk(mtx);

	for(unsigned int i=0;i<size;i++){
		char c = rbuf.at(i);
		fbuf[fp++] = c;
		if(c == '\n'){
			fp = 0;
			int f42_fx, f42_fy, f42_fz, f42_mx, f42_my, f42_mz;
			int f45_fx, f45_fy, f45_fz, f45_mx, f45_my, f45_mz;
			sscanf(fbuf, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\r\n",
				&f42_fx, &f42_fy, &f42_fz, &f42_mx, &f42_my, &f42_mz,
				&f45_fx, &f45_fy, &f45_fz, &f45_mx, &f45_my, &f45_mz);
			f42.rad_xy = atan2(f42_my, f42_mx);
			f45.rad_xy = atan2(f45_my, f45_mx);
			printf("% 3.2f, % 3.2f\r\n", f42.rad_xy, f45.rad_xy);
		}
	}

	port.async_read_some( buffer(rbuf), boost::bind(&read_callback, _1, _2 ));
}

void write_callback(const boost::system::error_code& e, std::size_t size )
{
	std::cout << "write :" << size << "byte[s]" << std::endl;
}

int main(int argc, char *argv[])
{
	const boost::thread thr_wait(&waitKeyPressed);

	//std::string wbuf = argv[1];
	ptime now = second_clock::local_time();
	std::string logname = to_iso_string(now) + std::string(".log");
	ofs.open(logname.c_str());

	port.set_option(serial_port_base::baud_rate(115200));
	port.set_option(serial_port_base::character_size(8));
	port.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none));
	port.set_option(serial_port_base::parity(serial_port_base::parity::none));
	port.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one));

	boost::thread thr_io(boost::bind(&io_service::run, &io));

	port.async_read_some( buffer(rbuf), boost::bind(&read_callback, _1, _2 ));

	//port.async_write_some( buffer(wbuf), boost::bind(&write_callback, _1, _2));

	while(isRunning){
		sleep(2);
	}

	return 0;
}
