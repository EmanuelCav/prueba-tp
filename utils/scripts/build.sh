#!/bin/bash

# Script para compilar todos los módulos del proyecto

set -e 

echo "=============================================="
echo "  COMPILANDO PROYECTO TP-2025-2c-SOGrupo3523"
echo "=============================================="

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Función para mostrar mensajes
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

if [ ! -f "README.md" ]; then
    log_error "Este script debe ejecutarse desde el directorio raíz del proyecto"
    exit 1
fi

log_info "Verificando dependencias..."

if ! command -v gcc &> /dev/null; then
    log_error "gcc no está instalado"
    exit 1
fi

if ! command -v make &> /dev/null; then
    log_error "make no está instalado"
    exit 1
fi

# Verificar que existe la librería commons
if [ ! -d "/usr/local/include/commons" ]; then
    log_warn "La librería commons no está instalada. Instalando..."
    if [ -d "so-commons-library" ]; then
        cd so-commons-library
        make debug
        make install
        cd ..
    else
        log_error "No se encontró so-commons-library. Clonando..."
        git clone https://github.com/sisoputnfrba/so-commons-library
        cd so-commons-library
        make debug
        make install
        cd ..
    fi
fi

# MODULOS BUILDEADOS POR ORDEN DE IMPORTANCIA-DEPENDENCIA
MODULES=("utils" "storage" "master" "worker" "query_control")

for module in "${MODULES[@]}"; do
    if [ ! -d "$module/logs" ]; then
        log_warn "Carpeta logs inexistente en $module."
        log_info "Creando carpeta logs en $module..."
        mkdir -p "$module/logs"
        log_info "✓ Carpeta logs creada en $module"
    fi

    cd $module
    make clean
    make debug

    if [ $? -eq 0 ]; then
        log_info "✓ $module compilado correctamente"
    else
        log_error "✗ Error compilando $module"
        exit 1
    fi

    cd ..
done

echo ""
echo "=========================================="
log_info "¡COMPILACIÓN COMPLETADA EXITOSAMENTE!"
echo "=========================================="
echo ""
echo "Para ejecutar el sistema completo, usa:"
echo "  ./start.sh"
echo ""
