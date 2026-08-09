class Address {
public:
  Address();

  Address(unsigned char a, unsigned char b, unsigned char c, unsigned char d,
          unsigned short port);

  Address(unsigned int address, unsigned short port);

  unsigned int GetAddress() const;

  unsigned char GetA();
  unsigned char GetB();
  unsigned char GetC();
  unsigned char GetD();
  unsigned short GetPort();

private:
  unsigned short port;
  unsigned int address;
};