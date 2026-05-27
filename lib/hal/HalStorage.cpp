#define HAL_STORAGE_IMPL
#include "HalStorage.h"

#include <FS.h>
#include <Logging.h>
#include <SD.h>
#include <SPI.h>
#include <TDeckMaxBoard.h>

#include <algorithm>
#include <cassert>

HalStorage HalStorage::instance;

namespace {
String dirnameOf(const char* path) {
  if (!path || path[0] == '\0') {
    return "/";
  }
  String full(path);
  const int slash = full.lastIndexOf('/');
  if (slash <= 0) {
    return "/";
  }
  return full.substring(0, slash);
}

String basenameOf(const String& path) {
  const int slash = path.lastIndexOf('/');
  if (slash < 0) {
    return path;
  }
  return path.substring(slash + 1);
}

String joinPath(const String& parent, const String& child) {
  if (child.startsWith("/")) {
    return child;
  }
  if (parent.isEmpty() || parent == "/") {
    return "/" + child;
  }
  return parent + "/" + child;
}

void deselectSharedSpiDevices() {
  pinMode(BOARD_LORA_CS, OUTPUT);
  digitalWrite(BOARD_LORA_CS, HIGH);
  pinMode(BOARD_LORA_RST, OUTPUT);
  digitalWrite(BOARD_LORA_RST, HIGH);
  pinMode(BOARD_EPD_CS, OUTPUT);
  digitalWrite(BOARD_EPD_CS, HIGH);
  pinMode(BOARD_SD_CS, OUTPUT);
  digitalWrite(BOARD_SD_CS, HIGH);
}

bool ensureDirectoryRecursive(fs::FS& fs, const char* path) {
  if (!path || path[0] == '\0' || strcmp(path, "/") == 0) {
    return true;
  }

  String normalized(path);
  if (!normalized.startsWith("/")) {
    normalized = "/" + normalized;
  }

  int start = 1;
  while (start < normalized.length()) {
    int slash = normalized.indexOf('/', start);
    const String partial = slash < 0 ? normalized : normalized.substring(0, slash);
    if (!partial.isEmpty() && !fs.exists(partial.c_str()) && !fs.mkdir(partial.c_str())) {
      return false;
    }
    if (slash < 0) {
      break;
    }
    start = slash + 1;
  }

  return true;
}

fs::File openWithFlags(fs::FS& fs, const char* path, const oflag_t oflag) {
  const bool wantsWrite = (oflag & (O_WRITE | O_RDWR | O_APPEND | O_CREAT | O_TRUNC)) != 0;
  if (!wantsWrite) {
    return fs.open(path, FILE_READ);
  }

  ensureDirectoryRecursive(fs, dirnameOf(path).c_str());
  if ((oflag & O_TRUNC) && fs.exists(path)) {
    fs.remove(path);
  }

  fs::File file = fs.open(path, FILE_WRITE);
  if (file && !(oflag & O_APPEND)) {
    file.seek(0, SeekSet);
  }
  return file;
}

String fileFullPath(const String& parentPath, fs::File& file) {
  const String rawName = file.name();
  return joinPath(parentPath, rawName);
}

bool removeRecursive(fs::FS& fs, const char* path) {
  fs::File file = fs.open(path, FILE_READ);
  if (!file) {
    return false;
  }

  if (!file.isDirectory()) {
    file.close();
    return fs.remove(path);
  }

  const String dirPath = path;
  fs::File entry = file.openNextFile();
  while (entry) {
    const String childPath = fileFullPath(dirPath, entry);
    const bool childIsDirectory = entry.isDirectory();
    entry.close();
    if (childIsDirectory) {
      if (!removeRecursive(fs, childPath.c_str())) {
        file.close();
        return false;
      }
    } else if (!fs.remove(childPath.c_str())) {
      file.close();
      return false;
    }
    entry = file.openNextFile();
  }

  file.close();
  return fs.rmdir(path);
}
}  // namespace

HalStorage::HalStorage() {
  storageMutex = xSemaphoreCreateMutex();
  assert(storageMutex != nullptr);
}

class HalStorage::StorageLock {
 public:
  StorageLock() { xSemaphoreTake(HalStorage::getInstance().storageMutex, portMAX_DELAY); }
  ~StorageLock() { xSemaphoreGive(HalStorage::getInstance().storageMutex); }
};

bool HalStorage::begin() {
  if (initialized) {
    return true;
  }

  SPI.begin(BOARD_SPI_SCK, BOARD_SPI_MISO, BOARD_SPI_MOSI);
  deselectSharedSpiDevices();
  initialized = SD.begin(BOARD_SD_CS, SPI);
  return initialized;
}

bool HalStorage::ready() const { return initialized && SD.cardType() != CARD_NONE; }

std::vector<String> HalStorage::listFiles(const char* path, const int maxFiles) {
  StorageLock lock;
  std::vector<String> files;

  fs::File dir = SD.open(path, FILE_READ);
  if (!dir || !dir.isDirectory()) {
    return files;
  }

  const String basePath(path);
  for (fs::File entry = dir.openNextFile(); entry && static_cast<int>(files.size()) < maxFiles; entry = dir.openNextFile()) {
    String name = basenameOf(entry.name());
    if (entry.isDirectory()) {
      name += "/";
    }
    files.push_back(name);
    entry.close();
  }

  return files;
}

String HalStorage::readFile(const char* path) {
  StorageLock lock;
  fs::File file = SD.open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    return "";
  }
  String result;
  while (file.available()) {
    result += static_cast<char>(file.read());
  }
  file.close();
  return result;
}

bool HalStorage::readFileToStream(const char* path, Print& out, const size_t chunkSize) {
  StorageLock lock;
  fs::File file = SD.open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    return false;
  }

  auto* buffer = static_cast<uint8_t*>(malloc(chunkSize));
  if (!buffer) {
    file.close();
    return false;
  }

  while (file.available()) {
    const size_t readCount = file.read(buffer, chunkSize);
    if (readCount == 0) {
      break;
    }
    out.write(buffer, readCount);
  }

  free(buffer);
  file.close();
  return true;
}

size_t HalStorage::readFileToBuffer(const char* path, char* buffer, const size_t bufferSize, const size_t maxBytes) {
  if (!buffer || bufferSize == 0) {
    return 0;
  }

  StorageLock lock;
  fs::File file = SD.open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    buffer[0] = '\0';
    return 0;
  }

  const size_t target = maxBytes > 0 ? std::min(bufferSize - 1, maxBytes) : (bufferSize - 1);
  const size_t readCount = file.readBytes(buffer, target);
  buffer[readCount] = '\0';
  file.close();
  return readCount;
}

bool HalStorage::writeFile(const char* path, const String& content) {
  StorageLock lock;
  ensureDirectoryRecursive(SD, dirnameOf(path).c_str());
  if (SD.exists(path)) {
    SD.remove(path);
  }

  fs::File file = SD.open(path, FILE_WRITE);
  if (!file) {
    return false;
  }
  const bool ok = file.write(reinterpret_cast<const uint8_t*>(content.c_str()), content.length()) == content.length();
  file.close();
  return ok;
}

bool HalStorage::ensureDirectoryExists(const char* path) {
  StorageLock lock;
  return ensureDirectoryRecursive(SD, path);
}

class HalFile::Impl {
 public:
  Impl(fs::File&& fsFile, const String& filePath, const bool writable)
      : file(std::move(fsFile)), path(filePath), writeMode(writable) {}

  fs::File file;
  String path;
  bool writeMode = false;
};

HalFile::HalFile() = default;

HalFile::HalFile(std::unique_ptr<Impl> impl) : impl(std::move(impl)) {}

HalFile::~HalFile() = default;

HalFile::HalFile(HalFile&&) = default;

HalFile& HalFile::operator=(HalFile&&) = default;

HalFile HalStorage::open(const char* path, const oflag_t oflag) {
  StorageLock lock;
  fs::File file = openWithFlags(SD, path, oflag);
  return HalFile(std::make_unique<HalFile::Impl>(std::move(file), String(path),
                                                 (oflag & (O_WRITE | O_RDWR | O_APPEND | O_CREAT | O_TRUNC)) != 0));
}

bool HalStorage::mkdir(const char* path, const bool pFlag) {
  StorageLock lock;
  return pFlag ? ensureDirectoryRecursive(SD, path) : SD.mkdir(path);
}

bool HalStorage::exists(const char* path) {
  StorageLock lock;
  return SD.exists(path);
}

bool HalStorage::remove(const char* path) {
  StorageLock lock;
  return SD.remove(path);
}

bool HalStorage::rename(const char* oldPath, const char* newPath) {
  StorageLock lock;
  ensureDirectoryRecursive(SD, dirnameOf(newPath).c_str());
  return SD.rename(oldPath, newPath);
}

bool HalStorage::rmdir(const char* path) {
  StorageLock lock;
  return SD.rmdir(path);
}

bool HalStorage::openFileForRead(const char* moduleName, const char* path, HalFile& file) {
  (void)moduleName;
  StorageLock lock;
  fs::File fsFile = SD.open(path, FILE_READ);
  file = HalFile(std::make_unique<HalFile::Impl>(std::move(fsFile), String(path), false));
  return file.isOpen();
}

bool HalStorage::openFileForRead(const char* moduleName, const std::string& path, HalFile& file) {
  return openFileForRead(moduleName, path.c_str(), file);
}

bool HalStorage::openFileForRead(const char* moduleName, const String& path, HalFile& file) {
  return openFileForRead(moduleName, path.c_str(), file);
}

bool HalStorage::openFileForWrite(const char* moduleName, const char* path, HalFile& file) {
  (void)moduleName;
  StorageLock lock;
  ensureDirectoryRecursive(SD, dirnameOf(path).c_str());
  if (SD.exists(path)) {
    SD.remove(path);
  }
  fs::File fsFile = SD.open(path, FILE_WRITE);
  file = HalFile(std::make_unique<HalFile::Impl>(std::move(fsFile), String(path), true));
  return file.isOpen();
}

bool HalStorage::openFileForWrite(const char* moduleName, const std::string& path, HalFile& file) {
  return openFileForWrite(moduleName, path.c_str(), file);
}

bool HalStorage::openFileForWrite(const char* moduleName, const String& path, HalFile& file) {
  return openFileForWrite(moduleName, path.c_str(), file);
}

bool HalStorage::removeDir(const char* path) {
  StorageLock lock;
  return removeRecursive(SD, path);
}

#define HAL_FILE_WRAPPED_CALL(method, ...) \
  HalStorage::StorageLock lock;            \
  assert(impl != nullptr);                 \
  return impl->file.method(__VA_ARGS__);

#define HAL_FILE_FORWARD_CALL(method, ...) \
  assert(impl != nullptr);                 \
  return impl->file.method(__VA_ARGS__);

void HalFile::flush() { HAL_FILE_WRAPPED_CALL(flush, ); }

size_t HalFile::getName(char* name, size_t len) {
  HalStorage::StorageLock lock;
  assert(impl != nullptr);
  const String baseName = basenameOf(impl->path);
  if (!name || len == 0) {
    return 0;
  }
  const size_t copyLen = std::min(len - 1, static_cast<size_t>(baseName.length()));
  memcpy(name, baseName.c_str(), copyLen);
  name[copyLen] = '\0';
  return copyLen;
}

size_t HalFile::size() { HAL_FILE_FORWARD_CALL(size, ); }

size_t HalFile::fileSize() { HAL_FILE_FORWARD_CALL(size, ); }

bool HalFile::seek(size_t pos) { return seekSet(pos); }

bool HalFile::seekCur(int64_t offset) {
  HalStorage::StorageLock lock;
  assert(impl != nullptr);
  const int64_t current = static_cast<int64_t>(impl->file.position());
  const int64_t target = std::max<int64_t>(0, current + offset);
  return impl->file.seek(static_cast<uint32_t>(target), SeekSet);
}

bool HalFile::seekSet(size_t offset) {
  HalStorage::StorageLock lock;
  assert(impl != nullptr);
  return impl->file.seek(static_cast<uint32_t>(offset), SeekSet);
}

int HalFile::available() const { HAL_FILE_WRAPPED_CALL(available, ); }

size_t HalFile::position() const { HAL_FILE_WRAPPED_CALL(position, ); }

int HalFile::read(void* buf, size_t count) { HAL_FILE_WRAPPED_CALL(read, static_cast<uint8_t*>(buf), count); }

int HalFile::read() { HAL_FILE_WRAPPED_CALL(read, ); }

size_t HalFile::write(const void* buf, size_t count) {
  HAL_FILE_WRAPPED_CALL(write, static_cast<const uint8_t*>(buf), count);
}

size_t HalFile::write(uint8_t b) { HAL_FILE_WRAPPED_CALL(write, b); }

bool HalFile::rename(const char* newPath) {
  HalStorage::StorageLock lock;
  assert(impl != nullptr);
  ensureDirectoryRecursive(SD, dirnameOf(newPath).c_str());

  const String oldPath = impl->path;
  impl->file.close();
  const bool ok = SD.rename(oldPath.c_str(), newPath);
  if (!ok) {
    impl->file = SD.open(oldPath.c_str(), impl->writeMode ? FILE_WRITE : FILE_READ);
    return false;
  }

  impl->path = newPath;
  impl->file = SD.open(newPath, impl->writeMode ? FILE_WRITE : FILE_READ);
  return true;
}

bool HalFile::isDirectory() const { HAL_FILE_FORWARD_CALL(isDirectory, ); }

void HalFile::rewindDirectory() { HAL_FILE_WRAPPED_CALL(rewindDirectory, ); }

bool HalFile::close() {
  HalStorage::StorageLock lock;
  if (!impl) {
    return false;
  }
  impl->file.close();
  return true;
}

HalFile HalFile::openNextFile() {
  HalStorage::StorageLock lock;
  assert(impl != nullptr);
  fs::File next = impl->file.openNextFile();
  const String nextPath = next ? fileFullPath(impl->path, next) : String();
  return HalFile(std::make_unique<Impl>(std::move(next), nextPath, false));
}

bool HalFile::isOpen() const { return impl != nullptr && impl->file; }

HalFile::operator bool() const { return isOpen(); }
