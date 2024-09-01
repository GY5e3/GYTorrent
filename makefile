# Компилятор и флаги
CXX = g++
CXXFLAGS = -std=c++20 -Wall -I/usr/include/boost -I/usr/local/include 
LDFLAGS = -L/usr/lib -L/usr/local/lib -ltorrent-rasterbar -lboost_system -lboost_coroutine -lcrypto -pthread#s-fsanitize=undefined,address

# Название исполняемого файла
TARGET = _GYTorrent

# Исходные файлы
SRC = main.cpp TorrentMetaData.cpp  utils.cpp PieceManager.cpp Connection.cpp Mediator.cpp

# Правило по умолчанию
all: $(TARGET)

# Правило для сборки исполняемого файла
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Правило для очистки сборки
clean:
	rm -f $(TARGET)

.PHONY: all clean
