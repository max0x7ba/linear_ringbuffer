#include <bev/linear_ringbuffer.hpp>
#include <bev/io_buffer.hpp>

#include <iostream>
#include <assert.h>

void print_mappings()
{
	pid_t pid = getpid();
	char cmd[128];
	sprintf(cmd, "cat /proc/%d/maps", pid);
	system(cmd);
}


void test_linear_ringbuffer() {
	bev::linear_ringbuffer rb(4095);
	int n = rb.capacity();
	assert(n % 4096 == 0); // Check rounding up to the minimum page size of 4096.
	assert(!rb.size());
	assert(rb.empty());
	// Test 1: Check that we can read and write the full capacity
	// of the buffer.
	std::cout << "Test 1..." << std::flush;
	char* sbuf = static_cast<char*>(malloc(rb.capacity()));
	char* tbuf = static_cast<char*>(malloc(rb.capacity()));
	std::fill_n(sbuf, n, 'x');
	std::fill_n(tbuf, n, '\0');

	assert(rb.free_size() == rb.capacity());
	::memcpy(rb.write_head(), sbuf, n);
	rb.commit(n);
	assert(rb.size() == n);
	assert(!rb.empty());

	::memcpy(tbuf, rb.read_head(), rb.size());
	rb.consume(n);

	assert(rb.size() == 0);

	std::fill_n(tbuf, n, 'x');
	std::cout << "success\n";

	// Test 2: Check than we can read and write "over the edge", i.e.
	// starting in one copy of the buffer and ending in the other.
	std::cout << "Test 2..." << std::flush;
	rb.clear();
	assert(rb.empty());
	std::fill_n(sbuf, n, 'y');
	rb.commit(n/2);
	rb.consume(n/2);
	assert(rb.empty());

	// Arbitrarily use some amount n/2 < m < n to ensure we write
	// over the edge.
	int m = n/2 + n/4;
	::memcpy(rb.write_head(), sbuf, m);
	rb.commit(m);

	assert(rb.size() == m);

	::memcpy(tbuf, rb.read_head(), m);
	rb.consume(m);

	assert(rb.size() == 0);
	for (int i=0; i<m; ++i) {
		assert(tbuf[i] == 'y');
	}

	std::cout << "success\n";

	// Test 3: Check that for-loop iteration works
	const char test3[] = "Test 3...success\n";
	rb.clear();
	::memcpy(rb.write_head(), test3, sizeof test3 - 1);
	rb.commit(sizeof test3 - 1);
	for (char c : rb) {
		std::cout << c;
	}
}

void test_io_buffer()
{
	bev::io_buffer iob(4096);
	int n = iob.capacity();

	// Test 1: Check that we can read and write the full capacity
	// of the buffer.
	std::cout << "Test 1..." << std::flush;
	char* sbuf = static_cast<char*>(malloc(iob.capacity()));
	char* tbuf = static_cast<char*>(malloc(iob.capacity()));
	std::fill_n(sbuf, n, 'x');
	std::fill_n(tbuf, n, '\0');

	assert(iob.free_size() == iob.capacity());
	::memcpy(iob.write_head(), sbuf, n);
	iob.commit(n);

	::memcpy(tbuf, iob.read_head(), iob.size());
	iob.consume(n);

	assert(iob.size() == 0);

	std::fill_n(tbuf, n, 'x');
	std::cout << "success\n";

	// Test 2: Check than we can read and write "over the edge", i.e.
	// starting in one copy of the buffer and ending in the other.
	std::cout << "Test 2..." << std::flush;
	iob.clear();
	std::fill_n(sbuf, n, 'y');
	iob.commit(n/2);
	iob.consume(n/2);

	// Arbitrarily use some amount n/2 < m < n to ensure we write
	// over the edge.
	int m = n/2 + n/4;
        auto slab = iob.prepare(m);
        assert(slab.size == m);

	::memcpy(slab.data, sbuf, slab.size);
	iob.commit(slab.size);
	assert(iob.size() == slab.size);

	::memcpy(tbuf, iob.read_head(), m);
	iob.consume(m);

	assert(iob.size() == 0);
	for (int i=0; i<m; ++i) {
		assert(tbuf[i] == 'y');
	}

	std::cout << "success\n";

	// Test 3: Use a custom memory region.
	std::cout << "Test 3..." << std::flush;

	static int deletes;
	deletes = 0;

        struct Deleter {
		Deleter() = default;
		// The standard says that "A deleter’s state need never be copied, only moved or
		// swapped as ownership changes." I'm not sure if that means that the state is not
		// allowed to be copied, or that it is not required to copy the state for a conforming
		// implementation. Let's assume the former to be safe.
		Deleter(const Deleter&) { throw std::runtime_error("dont copy me :("); }
		void operator()(char* c) { ++deletes; delete[] c; }
	} static_deleter;
	std::unique_ptr<char, Deleter&> p(new char[128], static_deleter);

	{
		bev::io_buffer buf(std::move(p), 128);
		assert(buf.free_size() == 128);
		assert(buf.capacity() == 128);
	}

	assert(deletes == 1);
	std::cout << "success\n";
}

int main()
{
	std::cout << "Testing linear_ringbuffer...\n";
	test_linear_ringbuffer();
	std::cout << "Testing io_ringbuffer...\n";
	test_io_buffer();
}
