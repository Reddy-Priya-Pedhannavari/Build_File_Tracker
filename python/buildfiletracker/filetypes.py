"""
buildfiletracker.filetypes - Comprehensive File Type Detection and Analysis

Tracks ALL file types used in projects:
- Source code (C, C++, Python, Java, JavaScript, Rust, Go, etc.)
- Binary files (object files, libraries, executables)
- Archives (ZIP, TAR, JAR, WAR, WHL, etc.)
- Resources (images, fonts, configs)
- Documentation, databases, certificates, and more

Copyright (c) 2026 BuildFileTracker Contributors
SPDX-License-Identifier: MIT
"""

import os
import sys
import struct
import hashlib
import subprocess
import platform
from enum import Enum
from typing import List, Set, Dict, Optional, Tuple
from dataclasses import dataclass, field
from pathlib import Path


class FileType(Enum):
    """Comprehensive file type enumeration"""
    
    UNKNOWN = "unknown"
    
    # Source Code
    C_SOURCE = "c_source"
    CPP_SOURCE = "cpp_source"
    C_HEADER = "c_header"
    ASSEMBLY = "assembly"
    
    # Binary/Object Files
    OBJECT = "object"
    STATIC_LIB = "static_lib"
    SHARED_LIB = "shared_lib"
    EXECUTABLE = "executable"
    
    # Archives
    ARCHIVE_ZIP = "archive_zip"
    ARCHIVE_TAR = "archive_tar"
    ARCHIVE_7Z = "archive_7z"
    ARCHIVE_RAR = "archive_rar"
    
    # Java/JVM
    JAVA_SOURCE = "java_source"
    JAVA_CLASS = "java_class"
    JAVA_JAR = "java_jar"
    JAVA_WAR = "java_war"
    JAVA_EAR = "java_ear"
    
    # Python
    PYTHON_SOURCE = "python_source"
    PYTHON_COMPILED = "python_compiled"
    PYTHON_WHEEL = "python_wheel"
    PYTHON_EGG = "python_egg"
    PYTHON_SO = "python_so"
    
    # JavaScript/Node
    JAVASCRIPT = "javascript"
    TYPESCRIPT = "typescript"
    NODE_ADDON = "node_addon"
    WASM = "wasm"
    
    # .NET
    CSHARP_SOURCE = "csharp_source"
    DOTNET_DLL = "dotnet_dll"
    DOTNET_EXE = "dotnet_exe"
    DOTNET_NUPKG = "dotnet_nupkg"
    
    # Rust
    RUST_SOURCE = "rust_source"
    RUST_LIB = "rust_lib"
    
    # Go
    GO_SOURCE = "go_source"
    GO_ARCHIVE = "go_archive"
    
    # Build/Config
    MAKEFILE = "makefile"
    CMAKE = "cmake"
    NINJA = "ninja"
    BAZEL = "bazel"
    GRADLE = "gradle"
    MAVEN = "maven"
    
    # Data/Config
    JSON = "json"
    XML = "xml"
    YAML = "yaml"
    TOML = "toml"
    INI = "ini"
    PROPERTIES = "properties"
    
    # Documentation
    MARKDOWN = "markdown"
    TEXT = "text"
    PDF = "pdf"
    HTML = "html"
    
    # Resources
    IMAGE = "image"
    FONT = "font"
    AUDIO = "audio"
    VIDEO = "video"
    
    # Database
    DATABASE = "database"
    SQL = "sql"
    
    # Security
    CERTIFICATE = "certificate"
    KEY = "key"
    
    # Package Formats
    DEB = "deb"
    RPM = "rpm"
    PKG = "pkg"
    MSI = "msi"
    APK = "apk"
    
    # Build Artifacts
    LLVM_BC = "llvm_bc"
    LLVM_IR = "llvm_ir"
    PRECOMPILED = "precompiled"
    DEBUG_INFO = "debug_info"


class FileCategory(Enum):
    """File category groupings"""
    SOURCE = "source"
    BINARY = "binary"
    LIBRARY = "library"
    ARCHIVE = "archive"
    CONFIG = "config"
    RESOURCE = "resource"
    DOCUMENTATION = "documentation"
    BUILD = "build"
    DATA = "data"
    PACKAGE = "package"


@dataclass
class FileInfo:
    """Complete file information"""
    filepath: str
    filetype: FileType
    categories: Set[FileCategory] = field(default_factory=set)
    
    # Metadata
    size: int = 0
    mtime: float = 0
    checksum_sha256: str = ""
    
    # Binary info
    is_binary: bool = False
    is_executable: bool = False
    is_library: bool = False
    is_32bit: bool = False
    is_64bit: bool = False
    
    # Dependencies
    dependencies: List[str] = field(default_factory=list)
    exported_symbols: List[str] = field(default_factory=list)
    imported_symbols: List[str] = field(default_factory=list)
    
    # Archive contents
    archive_contents: List[str] = field(default_factory=list)
    
    # Language/platform
    language: str = "unknown"
    platform: str = "unknown"
    architecture: str = "unknown"


class FileTypeDetector:
    """File type detection and analysis"""
    
    # Extension to FileType mapping
    EXTENSION_MAP = {
        # C/C++
        '.c': FileType.C_SOURCE,
        '.cpp': FileType.CPP_SOURCE,
        '.cc': FileType.CPP_SOURCE,
        '.cxx': FileType.CPP_SOURCE,
        '.C': FileType.CPP_SOURCE,
        '.c++': FileType.CPP_SOURCE,
        '.h': FileType.C_HEADER,
        '.hpp': FileType.C_HEADER,
        '.hxx': FileType.C_HEADER,
        '.hh': FileType.C_HEADER,
        '.h++': FileType.C_HEADER,
        '.s': FileType.ASSEMBLY,
        '.S': FileType.ASSEMBLY,
        '.asm': FileType.ASSEMBLY,
        
        # Object/Binary
        '.o': FileType.OBJECT,
        '.obj': FileType.OBJECT,
        '.a': FileType.STATIC_LIB,
        '.lib': FileType.STATIC_LIB,
        '.so': FileType.SHARED_LIB,
        '.dylib': FileType.SHARED_LIB,
        '.dll': FileType.SHARED_LIB,
        '.exe': FileType.EXECUTABLE,
        '.out': FileType.EXECUTABLE,
        
        # Archives
        '.zip': FileType.ARCHIVE_ZIP,
        '.tar': FileType.ARCHIVE_TAR,
        '.gz': FileType.ARCHIVE_TAR,
        '.tgz': FileType.ARCHIVE_TAR,
        '.bz2': FileType.ARCHIVE_TAR,
        '.7z': FileType.ARCHIVE_7Z,
        '.rar': FileType.ARCHIVE_RAR,
        
        # Java
        '.java': FileType.JAVA_SOURCE,
        '.class': FileType.JAVA_CLASS,
        '.jar': FileType.JAVA_JAR,
        '.war': FileType.JAVA_WAR,
        '.ear': FileType.JAVA_EAR,
        
        # Python
        '.py': FileType.PYTHON_SOURCE,
        '.pyc': FileType.PYTHON_COMPILED,
        '.pyo': FileType.PYTHON_COMPILED,
        '.whl': FileType.PYTHON_WHEEL,
        '.egg': FileType.PYTHON_EGG,
        '.pyd': FileType.PYTHON_SO,
        
        # JavaScript/TypeScript
        '.js': FileType.JAVASCRIPT,
        '.mjs': FileType.JAVASCRIPT,
        '.ts': FileType.TYPESCRIPT,
        '.tsx': FileType.TYPESCRIPT,
        '.node': FileType.NODE_ADDON,
        '.wasm': FileType.WASM,
        
        # .NET
        '.cs': FileType.CSHARP_SOURCE,
        '.nupkg': FileType.DOTNET_NUPKG,
        
        # Rust
        '.rs': FileType.RUST_SOURCE,
        '.rlib': FileType.RUST_LIB,
        
        # Go
        '.go': FileType.GO_SOURCE,
        
        # Data/Config
        '.json': FileType.JSON,
        '.xml': FileType.XML,
        '.yaml': FileType.YAML,
        '.yml': FileType.YAML,
        '.toml': FileType.TOML,
        '.ini': FileType.INI,
        '.conf': FileType.INI,
        '.cfg': FileType.INI,
        '.properties': FileType.PROPERTIES,
        
        # Documentation
        '.md': FileType.MARKDOWN,
        '.txt': FileType.TEXT,
        '.pdf': FileType.PDF,
        '.html': FileType.HTML,
        '.htm': FileType.HTML,
        
        # Resources
        '.png': FileType.IMAGE,
        '.jpg': FileType.IMAGE,
        '.jpeg': FileType.IMAGE,
        '.gif': FileType.IMAGE,
        '.svg': FileType.IMAGE,
        '.ico': FileType.IMAGE,
        '.bmp': FileType.IMAGE,
        '.ttf': FileType.FONT,
        '.otf': FileType.FONT,
        '.woff': FileType.FONT,
        '.woff2': FileType.FONT,
        
        # Database
        '.db': FileType.DATABASE,
        '.sqlite': FileType.DATABASE,
        '.sqlite3': FileType.DATABASE,
        '.sql': FileType.SQL,
        
        # Security
        '.pem': FileType.CERTIFICATE,
        '.crt': FileType.CERTIFICATE,
        '.cer': FileType.CERTIFICATE,
        '.der': FileType.CERTIFICATE,
        '.key': FileType.KEY,
        '.pub': FileType.KEY,
        
        # Packages
        '.deb': FileType.DEB,
        '.rpm': FileType.RPM,
        '.pkg': FileType.PKG,
        '.msi': FileType.MSI,
        '.apk': FileType.APK,
        
        # Build artifacts
        '.bc': FileType.LLVM_BC,
        '.ll': FileType.LLVM_IR,
        '.pch': FileType.PRECOMPILED,
        '.gch': FileType.PRECOMPILED,
        '.pdb': FileType.DEBUG_INFO,
    }
    
    # FileType to categories mapping
    TYPE_CATEGORIES = {
        FileType.C_SOURCE: {FileCategory.SOURCE},
        FileType.CPP_SOURCE: {FileCategory.SOURCE},
        FileType.C_HEADER: {FileCategory.SOURCE},
        FileType.PYTHON_SOURCE: {FileCategory.SOURCE},
        FileType.JAVA_SOURCE: {FileCategory.SOURCE},
        FileType.JAVASCRIPT: {FileCategory.SOURCE},
        FileType.TYPESCRIPT: {FileCategory.SOURCE},
        
        FileType.OBJECT: {FileCategory.BINARY},
        FileType.EXECUTABLE: {FileCategory.BINARY},
        
        FileType.STATIC_LIB: {FileCategory.BINARY, FileCategory.LIBRARY},
        FileType.SHARED_LIB: {FileCategory.BINARY, FileCategory.LIBRARY},
        
        FileType.ARCHIVE_ZIP: {FileCategory.ARCHIVE},
        FileType.ARCHIVE_TAR: {FileCategory.ARCHIVE},
        FileType.JAVA_JAR: {FileCategory.ARCHIVE, FileCategory.LIBRARY},
        FileType.PYTHON_WHEEL: {FileCategory.ARCHIVE, FileCategory.PACKAGE},
        
        FileType.JSON: {FileCategory.CONFIG, FileCategory.DATA},
        FileType.XML: {FileCategory.CONFIG, FileCategory.DATA},
        FileType.YAML: {FileCategory.CONFIG, FileCategory.DATA},
        
        FileType.MARKDOWN: {FileCategory.DOCUMENTATION},
        FileType.TEXT: {FileCategory.DOCUMENTATION},
        FileType.PDF: {FileCategory.DOCUMENTATION},
        
        FileType.IMAGE: {FileCategory.RESOURCE},
        FileType.FONT: {FileCategory.RESOURCE},
        
        FileType.MAKEFILE: {FileCategory.BUILD},
        FileType.CMAKE: {FileCategory.BUILD},
        
        FileType.DEB: {FileCategory.PACKAGE},
        FileType.RPM: {FileCategory.PACKAGE},
    }
    
    # Magic bytes for binary detection
    MAGIC_BYTES = {
        b'\x7fELF': 'ELF',           # Linux binary
        b'MZ': 'PE',                 # Windows executable
        b'\xca\xfe\xba\xbe': 'Mach-O',  # macOS binary (32-bit)
        b'\xcf\xfa\xed\xfe': 'Mach-O',  # macOS binary (64-bit)
        b'PK\x03\x04': 'ZIP',        # ZIP/JAR/APK
        b'\x1f\x8b': 'GZIP',         # GZIP
        b'Rar!': 'RAR',              # RAR
    }
    
    @classmethod
    def detect_type(cls, filepath: str) -> FileType:
        """Detect file type from extension and magic bytes"""
        path = Path(filepath)
        
        # Check special filenames
        filename = path.name.lower()
        if filename in ('makefile', 'gnumakefile'):
            return FileType.MAKEFILE
        if filename == 'cmakelists.txt':
            return FileType.CMAKE
        if filename == 'build.ninja':
            return FileType.NINJA
        if filename in ('build', 'build.bazel'):
            return FileType.BAZEL
        if filename == 'pom.xml':
            return FileType.MAVEN
        
        # Check extension
        ext = path.suffix.lower()
        if ext in cls.EXTENSION_MAP:
            return cls.EXTENSION_MAP[ext]
        
        # For .gradle files
        if ext == '.gradle' or filename == 'build.gradle':
            return FileType.GRADLE
        
        # Check double extensions (.tar.gz, etc.)
        if len(path.suffixes) >= 2:
            double_ext = ''.join(path.suffixes[-2:]).lower()
            if double_ext in cls.EXTENSION_MAP:
                return cls.EXTENSION_MAP[double_ext]
        
        # Try magic bytes for binary detection
        try:
            with open(filepath, 'rb') as f:
                header = f.read(4)
                for magic, type_name in cls.MAGIC_BYTES.items():
                    if header.startswith(magic):
                        if type_name == 'ELF':
                            return cls._detect_elf_type(filepath)
                        elif type_name == 'PE':
                            return cls._detect_pe_type(filepath)
                        elif type_name == 'ZIP':
                            # Could be JAR, APK, or plain ZIP
                            if ext == '.jar':
                                return FileType.JAVA_JAR
                            elif ext == '.apk':
                                return FileType.APK
                            return FileType.ARCHIVE_ZIP
        except:
            pass
        
        return FileType.UNKNOWN
    
    @classmethod
    def _detect_elf_type(cls, filepath: str) -> FileType:
        """Detect ELF binary type (.so, .o, or executable)"""
        try:
            result = subprocess.run(['file', filepath], 
                                   capture_output=True, text=True)
            output = result.stdout.lower()
            
            if 'shared object' in output or '.so' in filepath:
                return FileType.SHARED_LIB
            elif 'relocatable' in output:
                return FileType.OBJECT
            elif 'executable' in output:
                return FileType.EXECUTABLE
        except:
            pass
        
        # Fallback to extension
        if filepath.endswith('.so') or '.so.' in filepath:
            return FileType.SHARED_LIB
        elif filepath.endswith('.o'):
            return FileType.OBJECT
        
        return FileType.EXECUTABLE
    
    @classmethod
    def _detect_pe_type(cls, filepath: str) -> FileType:
        """Detect Windows PE type (.dll, .exe)"""
        ext = Path(filepath).suffix.lower()
        if ext == '.dll':
            return FileType.SHARED_LIB
        elif ext == '.exe':
            return FileType.EXECUTABLE
        return FileType.UNKNOWN
    
    @classmethod
    def get_categories(cls, filetype: FileType) -> Set[FileCategory]:
        """Get categories for file type"""
        return cls.TYPE_CATEGORIES.get(filetype, set())
    
    @classmethod
    def analyze_file(cls, filepath: str) -> FileInfo:
        """Complete file analysis"""
        info = FileInfo(
            filepath=filepath,
            filetype=cls.detect_type(filepath)
        )
        
        info.categories = cls.get_categories(info.filetype)
        
        # Get file stats
        try:
            stat = os.stat(filepath)
            info.size = stat.st_size
            info.mtime = stat.st_mtime
            info.is_executable = os.access(filepath, os.X_OK)
        except:
            pass
        
        # Calculate checksum for binaries and libraries
        if FileCategory.BINARY in info.categories or FileCategory.LIBRARY in info.categories:
            info.checksum_sha256 = cls._calculate_sha256(filepath)
            info.is_binary = True
        
        # Analyze binary dependencies
        if info.filetype in (FileType.SHARED_LIB, FileType.EXECUTABLE):
            info.dependencies = cls._get_binary_dependencies(filepath)
            info.is_library = (info.filetype == FileType.SHARED_LIB)
            info.architecture = cls._detect_architecture(filepath)
            
        # Analyze archive contents
        if FileCategory.ARCHIVE in info.categories:
            info.archive_contents = cls._list_archive_contents(filepath)
        
        # Set language
        info.language = cls._detect_language(info.filetype)
        
        return info
    
    @classmethod
    def _calculate_sha256(cls, filepath: str) -> str:
        """Calculate SHA256 checksum"""
        try:
            sha256 = hashlib.sha256()
            with open(filepath, 'rb') as f:
                for chunk in iter(lambda: f.read(8192), b''):
                    sha256.update(chunk)
            return sha256.hexdigest()
        except:
            return ""
    
    @classmethod
    def _get_binary_dependencies(cls, filepath: str) -> List[str]:
        """Get binary dependencies (linked libraries) - cross-platform"""
        deps = []
        system = platform.system()
        
        # Linux: Use ldd
        if system == 'Linux':
            try:
                result = subprocess.run(['ldd', filepath],
                                       capture_output=True, text=True,
                                       timeout=5)
                if result.returncode == 0:
                    for line in result.stdout.split('\n'):
                        if '=>' in line:
                            parts = line.split('=>')
                            if len(parts) >= 2:
                                dep = parts[0].strip()
                                deps.append(dep)
            except:
                pass
        
        # macOS: Use otool
        elif system == 'Darwin':
            try:
                result = subprocess.run(['otool', '-L', filepath],
                                       capture_output=True, text=True,
                                       timeout=5)
                if result.returncode == 0:
                    lines = result.stdout.split('\n')[1:]  # Skip first line
                    for line in lines:
                        if line.strip():
                            dep = line.strip().split()[0]
                            deps.append(dep)
            except:
                pass
        
        # Windows: Use dumpbin or objdump
        elif system == 'Windows':
            # Try dumpbin (Visual Studio)
            try:
                result = subprocess.run(['dumpbin', '/DEPENDENTS', filepath],
                                       capture_output=True, text=True,
                                       timeout=5, shell=True)
                if result.returncode == 0:
                    in_deps = False
                    for line in result.stdout.split('\n'):
                        line = line.strip()
                        if 'Image has the following dependencies' in line:
                            in_deps = True
                            continue
                        if in_deps:
                            if line and not line.startswith('Summary'):
                                if line.endswith('.dll'):
                                    deps.append(line)
                            else:
                                break
            except:
                pass
            
            # Try objdump (MinGW/Cygwin)
            if not deps:
                try:
                    result = subprocess.run(['objdump', '-p', filepath],
                                           capture_output=True, text=True,
                                           timeout=5)
                    if result.returncode == 0:
                        for line in result.stdout.split('\n'):
                            if 'DLL Name:' in line:
                                dep = line.split('DLL Name:')[-1].strip()
                                deps.append(dep)
                except:
                    pass
        
        return deps
    
    @classmethod
    def _detect_architecture(cls, filepath: str) -> str:
        """Detect binary architecture - supports all major CPU architectures"""
        system = platform.system()
        
        # Try reading ELF/PE/Mach-O headers directly
        arch = cls._detect_arch_from_binary(filepath)
        if arch != 'unknown':
            return arch
        
        # Fallback to 'file' command (Unix-like systems)
        if system in ('Linux', 'Darwin'):
            try:
                result = subprocess.run(['file', filepath],
                                       capture_output=True, text=True,
                                       timeout=2)
                output = result.stdout.lower()
                
                # x86/x64
                if 'x86-64' in output or 'x86_64' in output or 'amd64' in output:
                    return 'x86_64'
                elif 'x86' in output or 'i386' in output or 'i686' in output:
                    return 'x86'
                
                # ARM
                elif 'arm64' in output or 'aarch64' in output:
                    return 'arm64'
                elif 'armv7' in output:
                    return 'armv7'
                elif 'arm' in output:
                    return 'arm'
                
                # RISC-V
                elif 'riscv64' in output or 'risc-v 64' in output:
                    return 'riscv64'
                elif 'riscv32' in output or 'risc-v 32' in output:
                    return 'riscv32'
                
                # MIPS
                elif 'mips64' in output:
                    return 'mips64'
                elif 'mips' in output:
                    return 'mips'
                
                # PowerPC
                elif 'powerpc64' in output or 'ppc64' in output:
                    return 'powerpc64'
                elif 'powerpc' in output or 'ppc' in output:
                    return 'powerpc'
                
                # SPARC
                elif 'sparc64' in output:
                    return 'sparc64'
                elif 'sparc' in output:
                    return 'sparc'
                
                # IBM s390x
                elif 's390x' in output or 's390' in output:
                    return 's390x'
                
                # LoongArch
                elif 'loongarch64' in output:
                    return 'loongarch64'
                
            except:
                pass
        
        # Windows: Use dumpbin
        elif system == 'Windows':
            try:
                result = subprocess.run(['dumpbin', '/HEADERS', filepath],
                                       capture_output=True, text=True,
                                       timeout=2, shell=True)
                output = result.stdout.lower()
                
                if 'x64' in output or 'x86-64' in output:
                    return 'x86_64'
                elif 'x86' in output or 'i386' in output:
                    return 'x86'
                elif 'arm64' in output or 'aarch64' in output:
                    return 'arm64'
                elif 'arm' in output:
                    return 'arm'
            except:
                pass
        
        return 'unknown'
    
    @classmethod
    def _detect_arch_from_binary(cls, filepath: str) -> str:
        """Detect architecture directly from binary headers"""
        try:
            with open(filepath, 'rb') as f:
                magic = f.read(4)
                
                # ELF (Linux/Unix)
                if magic[:4] == b'\x7fELF':
                    f.seek(18)  # e_machine offset
                    machine = struct.unpack('<H', f.read(2))[0]
                    
                    elf_machines = {
                        0x03: 'x86',           # EM_386
                        0x3E: 'x86_64',        # EM_X86_64
                        0x28: 'arm',           # EM_ARM
                        0xB7: 'arm64',         # EM_AARCH64
                        0xF3: 'riscv64',       # EM_RISCV (check class for 32/64)
                        0x08: 'mips',          # EM_MIPS
                        0x14: 'powerpc',       # EM_PPC
                        0x15: 'powerpc64',     # EM_PPC64
                        0x2B: 'sparc',         # EM_SPARC32PLUS
                        0x12: 'sparc64',       # EM_SPARCV9
                        0x16: 's390x',         # EM_S390
                        0x102: 'loongarch64',  # EM_LOONGARCH
                    }
                    
                    if machine in elf_machines:
                        return elf_machines[machine]
                
                # PE (Windows)
                elif magic[:2] == b'MZ':
                    f.seek(0x3C)  # Offset to PE header
                    pe_offset = struct.unpack('<I', f.read(4))[0]
                    f.seek(pe_offset + 4)  # Machine type
                    machine = struct.unpack('<H', f.read(2))[0]
                    
                    pe_machines = {
                        0x014c: 'x86',       # IMAGE_FILE_MACHINE_I386
                        0x8664: 'x86_64',    # IMAGE_FILE_MACHINE_AMD64
                        0x01c0: 'arm',       # IMAGE_FILE_MACHINE_ARM
                        0xaa64: 'arm64',     # IMAGE_FILE_MACHINE_ARM64
                    }
                    
                    if machine in pe_machines:
                        return pe_machines[machine]
                
                # Mach-O (macOS)
                elif magic[:4] in (b'\xfe\xed\xfa\xce', b'\xce\xfa\xed\xfe',
                                  b'\xfe\xed\xfa\xcf', b'\xcf\xfa\xed\xfe'):
                    f.seek(4)  # CPU type
                    cpu_type = struct.unpack('<I', f.read(4))[0]
                    
                    macho_cpus = {
                        7: 'x86',            # CPU_TYPE_X86
                        0x01000007: 'x86_64', # CPU_TYPE_X86_64
                        12: 'arm',           # CPU_TYPE_ARM
                        0x0100000C: 'arm64', # CPU_TYPE_ARM64
                    }
                    
                    if cpu_type in macho_cpus:
                        return macho_cpus[cpu_type]
        
        except:
            pass
        
        return 'unknown'
    
    @classmethod
    def _list_archive_contents(cls, filepath: str) -> List[str]:
        """List contents of archive files"""
        contents = []
        ext = Path(filepath).suffix.lower()
        
        try:
            if ext in ('.zip', '.jar', '.whl', '.egg', '.apk'):
                import zipfile
                with zipfile.ZipFile(filepath, 'r') as zf:
                    contents = zf.namelist()
            
            elif ext in ('.tar', '.gz', '.tgz', '.bz2'):
                import tarfile
                with tarfile.open(filepath, 'r:*') as tf:
                    contents = tf.getnames()
        except:
            pass
        
        return contents
    
    @classmethod
    def _detect_language(cls, filetype: FileType) -> str:
        """Detect programming language from file type"""
        language_map = {
            FileType.C_SOURCE: 'C',
            FileType.CPP_SOURCE: 'C++',
            FileType.C_HEADER: 'C/C++',
            FileType.PYTHON_SOURCE: 'Python',
            FileType.JAVA_SOURCE: 'Java',
            FileType.JAVASCRIPT: 'JavaScript',
            FileType.TYPESCRIPT: 'TypeScript',
            FileType.CSHARP_SOURCE: 'C#',
            FileType.RUST_SOURCE: 'Rust',
            FileType.GO_SOURCE: 'Go',
        }
        return language_map.get(filetype, 'unknown')


# Extension presets for filtering
EXTENSIONS_SOURCE = ['.c', '.cpp', '.cc', '.cxx', '.h', '.hpp', '.hxx', 
                    '.py', '.java', '.js', '.ts', '.rs', '.go', '.cs']

EXTENSIONS_C_CPP = ['.c', '.cpp', '.cc', '.cxx', '.C', '.h', '.hpp', '.hxx', '.hh']

EXTENSIONS_BINARY = ['.o', '.obj', '.so', '.dll', '.dylib', '.exe', '.a', '.lib']

EXTENSIONS_LIBRARY = ['.so', '.dll', '.dylib', '.a', '.lib', '.jar']

EXTENSIONS_PYTHON = ['.py', '.pyc', '.pyo', '.pyd', '.whl', '.egg']

EXTENSIONS_JAVA = ['.java', '.class', '.jar', '.war', '.ear']

EXTENSIONS_JAVASCRIPT = ['.js', '.mjs', '.ts', '.tsx', '.node', '.wasm']

EXTENSIONS_CONFIG = ['.json', '.xml', '.yaml', '.yml', '.toml', '.ini', '.conf', '.cfg']

EXTENSIONS_RESOURCE = ['.png', '.jpg', '.jpeg', '.gif', '.svg', '.ico', 
                      '.ttf', '.otf', '.woff', '.woff2']

EXTENSIONS_ALL = ['*']  # Track everything
