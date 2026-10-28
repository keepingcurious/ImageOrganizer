#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <iomanip>
#include <future>
#include <mutex>
#include <exiv2/exiv2.hpp>
#include <CommonCrypto/CommonDigest.h>

namespace fs = std::filesystem;

// Thread-Sicherheit für globale Ressourcen
std::mutex logMutex;
std::mutex hashSetMutex;
std::mutex statsMutex;

struct Stats {
    long long totalFiles = 0;
    long long dupsFound = 0;
    long long bytesSaved = 0;
};

// Schreibt in Konsole und Logdatei gleichzeitig
void writeLog(std::ofstream& logFile, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) logFile << message << std::endl;
    std::cout << message << std::endl;
}

// MD5 Hash mit Apple CommonCrypto (nutzt Hardware-Beschleunigung)
std::string getMD5(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return "";
    CC_MD5_CTX md5Context;
    CC_MD5_Init(&md5Context);
    char buffer[16384]; // Optimierter Buffer für SSD-Durchsatz
    while (file.read(buffer, sizeof(buffer))) {
        CC_MD5_Update(&md5Context, buffer, (CC_LONG)file.gcount());
    }
    CC_MD5_Update(&md5Context, buffer, (CC_LONG)file.gcount());
    unsigned char result[CC_MD5_DIGEST_LENGTH];
    CC_MD5_Final(result, &md5Context);
    std::stringstream ss;
    for (int i = 0; i < CC_MD5_DIGEST_LENGTH; i++) ss << std::hex << std::setw(2) << std::setfill('0') << (int)result[i];
    return ss.str();
}

// Datum aus EXIF (v0.28+) oder Dateisystem
std::pair<std::string, std::string> getExifDate(const fs::path& p) {
    try {
        Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open(p.string());
        if (image.get()) {
            image->readMetadata();
            Exiv2::ExifData &exifData = image->exifData();
            Exiv2::ExifKey key("Exif.Photo.DateTimeOriginal");
            auto it = exifData.findKey(key);
            if (it != exifData.end() && it->count() > 0) {
                std::string d = it->toString();
                return { d.substr(0, 4), d.substr(5, 2) };
            }
        }
    } catch (...) {}
    
    // Fallback auf Filesystem-Zeit
    auto ftime = fs::last_write_time(p);
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    std::tm* time = std::localtime(&tt);
    char mb[3]; std::strftime(mb, 3, "%m", time);
    return { std::to_string(time->tm_year + 1900), std::string(mb) };
}

// Hauptfunktion für die Dateiverarbeitung (wird parallel ausgeführt)
void processFile(fs::path filePath, fs::path targetRoot, fs::path dupDir, std::set<std::string>& knownHashes, std::ofstream& logFile, Stats& stats) {
    std::string hash = getMD5(filePath.string());
    if (hash.empty()) return;

    // Duplikaterkennung
    {
        std::lock_guard<std::mutex> lock(hashSetMutex);
        if (knownHashes.count(hash)) {
            fs::create_directories(dupDir);
            fs::path dPath = dupDir / filePath.filename();
            
            // Name bei Konflikt im Duplikatordner anpassen
            int i = 1;
            while(fs::exists(dPath)) {
                dPath = dupDir / (filePath.stem().string() + "_" + std::to_string(i++) + filePath.extension().string());
            }
            
            fs::copy_file(filePath, dPath);
            writeLog(logFile, "[DUP] " + filePath.filename().string() + " verschoben.");
            
            std::lock_guard<std::mutex> sLock(statsMutex);
            stats.dupsFound++;
            stats.bytesSaved += fs::file_size(filePath);
            return;
        }
        knownHashes.insert(hash);
    }

    // Zielpfad bestimmen
    auto [year, month] = getExifDate(filePath);
    fs::path finalDir = targetRoot / year / month;
    fs::create_directories(finalDir);
    fs::path finalPath = finalDir / filePath.filename();

    // Kopieren und Zeitstempel-Synchronisation
    try {
        fs::copy_file(filePath, finalPath, fs::copy_options::overwrite_existing);

        // 1. Änderungsdatum (mtime)
        auto originalTime = fs::last_write_time(filePath);
        fs::last_write_time(finalPath, originalTime);

        // 2. Erstellungsdatum (birthtime) für macOS Finder
        std::string cmd = "setfile -d \"$(GetFileInfo -d '" + filePath.string() + "')\" '" + finalPath.string() + "' 2>/dev/null";
        system(cmd.c_str());

        writeLog(logFile, "[OK]  " + filePath.filename().string() + " -> " + year + "/" + month);
        
        std::lock_guard<std::mutex> sLock(statsMutex);
        stats.totalFiles++;
    } catch (const std::exception& e) {
        writeLog(logFile, "[ERR] Fehler bei " + filePath.filename().string() + ": " + e.what());
    }
}

int main() {
    std::string inStr, outStr;
    
    std::cout << "--- Apple M2 Max Image Sorter ---\n";
    while (true) {
        std::cout << "Quellordner: "; std::getline(std::cin, inStr);
        if (!inStr.empty() && fs::exists(inStr)) break;
    }
    while (true) {
        std::cout << "Zielordner:  "; std::getline(std::cin, outStr);
        if (!outStr.empty()) { fs::create_directories(outStr); break; }
    }

    auto start = std::chrono::high_resolution_clock::now();
    fs::path in(inStr), out(outStr);
    std::ofstream logFile(out / "sort_log.txt");
    std::set<std::string> knownHashes;
    std::vector<std::future<void>> futures;
    Stats stats;

    writeLog(logFile, "Indiziere Dateien und starte parallele Verarbeitung...");

    // Ermittle die Anzahl der Hardware-Kerne (beim M2 meist 8)
    // Wir nutzen das 2-fache der Kerne als Thread-Limit für I/O-Aufgaben
    const size_t maxThreads = std::thread::hardware_concurrency() * 3;

    writeLog(logFile, "M2-Kerne erkannt: " + std::to_string(std::thread::hardware_concurrency()));
    writeLog(logFile, "Thread-Limit gesetzt auf: " + std::to_string(maxThreads));

    for (const auto& entry : fs::recursive_directory_iterator(in)) {
        if (entry.is_regular_file()) {
            // Kontrollierter Thread-Pool: Wenn Limit erreicht, warte auf den ältesten Task
            if (futures.size() >= maxThreads) {
                if (futures.front().valid()) {
                    futures.front().get(); // Blockiert kurz, bis ein Slot frei wird
                }
                futures.erase(futures.begin());
            }

            // Starte neuen Task
            futures.push_back(std::async(std::launch::async, processFile,
                              entry.path(), out, out / "_DUPLIKATE",
                              std::ref(knownHashes), std::ref(logFile), std::ref(stats)));
        }
    }

    // Alle verbleibenden Tasks einsammeln
    for (auto& f : futures) {
        if (f.valid()) f.get();
    }

    // Abschlussstatistik
    std::stringstream res;
    res << "\n--- STATISTIK ---\n"
        << "Erfolgreich sortiert: " << stats.totalFiles << "\n"
        << "Duplikate aussortiert: " << stats.dupsFound << "\n"
        << "Speicherplatz gespart: " << std::fixed << std::setprecision(2) << (double)stats.bytesSaved / (1024 * 1024) << " MB\n"
        << "Logdatei: " << (out / "sort_log.txt").string() << "\n";
    
    writeLog(logFile, res.str());
    auto end = std::chrono::high_resolution_clock::now();
    // Dauer berechnen
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Ausführungszeit: " << duration.count() << " Mikrosekunden" << std::endl;
    
    return 0;
}
