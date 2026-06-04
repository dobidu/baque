#!/usr/bin/env bash
# Executa pluginval contra o BAQUE.vst3 construído.
# Baixa e verifica o binário se ausente (versão fixada + sha256).
set -euo pipefail

PLUGINVAL_VERSION="v1.0.4"
PLUGINVAL_SHA256="c01c49d8063965c4c2dea8324468336768f5c9139e0b1caebde14c2400b55352"
PLUGINVAL_URL="https://github.com/Tracktion/pluginval/releases/download/${PLUGINVAL_VERSION}/pluginval_Linux.zip"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGINVAL_BIN="${SCRIPT_DIR}/pluginval"
VST3_PATH="${SCRIPT_DIR}/../build/BAQUE_artefacts/Release/VST3/BAQUE.vst3"

# Baixar e verificar se não existe
if [[ ! -x "${PLUGINVAL_BIN}" ]]; then
    echo "Baixando pluginval ${PLUGINVAL_VERSION}..."
    TMPZIP="$(mktemp /tmp/pluginval-XXXXXX.zip)"
    curl -fsSL "${PLUGINVAL_URL}" -o "${TMPZIP}"

    # Verificar integridade
    ACTUAL_SHA=$(sha256sum "${TMPZIP}" | cut -d' ' -f1)
    if [[ "${ACTUAL_SHA}" != "${PLUGINVAL_SHA256}" ]]; then
        echo "ERRO: sha256 não corresponde!"
        echo "  Esperado: ${PLUGINVAL_SHA256}"
        echo "  Obtido:   ${ACTUAL_SHA}"
        rm -f "${TMPZIP}"
        exit 1
    fi

    unzip -q "${TMPZIP}" -d "${SCRIPT_DIR}"
    chmod +x "${PLUGINVAL_BIN}"
    rm -f "${TMPZIP}"
    echo "pluginval ${PLUGINVAL_VERSION} instalado e verificado."
fi

# Validar plugin (xvfb-run se não houver display)
if [[ -z "${DISPLAY:-}" ]]; then
    RUN_CMD="xvfb-run -a"
else
    RUN_CMD=""
fi

echo "Validando ${VST3_PATH} ..."
${RUN_CMD} "${PLUGINVAL_BIN}" \
    --validate "${VST3_PATH}" \
    --strictness-level 5 \
    --validate-in-process \
    --output-dir /tmp/pluginval-results

echo "pluginval: PASS"
