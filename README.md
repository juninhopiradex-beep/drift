# Drift

Plugin VST3 e AU para micro-variações naturais em música gerada por IA (foco em faixas do Suno).

## Módulos

| Módulo  | Função                                              |
|---------|-----------------------------------------------------|
| Groove  | Micro-deslocamento de timing (até 8 ms)             |
| Attack  | Variação de ganho por transiente                    |
| Air     | Deriva lenta de agudos, suaviza shimmer metálico    |
| Grit    | Textura de ruído subtil                             |
| Amount  | Quantidade global de processamento                  |

Modos: Livre, Sync, Adaptativo.

## Estado atual

Primeira versão do esqueleto. O DSP é uma aproximação simples e precisa de
validação com áudio real antes de ser considerado final.

## Build

Requer CMake 3.22+ e um compilador C++17.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

O JUCE é obtido automaticamente via FetchContent.

- macOS: os targets VST3 e AU ficam em `build/Drift_artefacts/Release/`.
- Windows: o target VST3 fica em `build\Drift_artefacts\Release\VST3\`.

## Validação

- AU: `auval -v aufx Drft Pirx` (no macOS).
- VST3: `pluginval` (https://github.com/Tracktion/pluginval).

## Créditos

Criador e Desenvolvedor Ricardo Amorim / Juninho Piradex.
