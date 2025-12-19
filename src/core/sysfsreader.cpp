// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2024 cpupower-gui contributors

#include "sysfsreader.h"

#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QRegularExpression>

SysfsReader::SysfsReader(QObject *parent)
    : QObject(parent)
{
}

QString SysfsReader::readFile(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream stream(&file);
    return stream.readAll().trimmed();
}

QStringList SysfsReader::parseList(const QString &content) const
{
    if (content.isEmpty()) {
        return QStringList();
    }

    return content.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
}

QList<int> SysfsReader::parseCpuList(const QString &content) const
{
    QList<int> result;
    if (content.isEmpty()) {
        return result;
    }

    // Parse format like "0,2,4-10,12" into a list of integers
    const QStringList parts = content.split(QLatin1Char(','));
    for (const QString &part : parts) {
        if (part.contains(QLatin1Char('-'))) {
            const QStringList range = part.split(QLatin1Char('-'));
            if (range.size() == 2) {
                int start = range[0].toInt();
                int end = range[1].toInt();
                for (int i = start; i <= end; ++i) {
                    result.append(i);
                }
            }
        } else {
            result.append(part.toInt());
        }
    }

    return result;
}

QString SysfsReader::cpuPath(int cpu) const
{
    return QStringLiteral("%1/cpu%2/%3")
        .arg(QLatin1String(SYS_CPU_PATH))
        .arg(cpu)
        .arg(QLatin1String(CPUFREQ_PATH));
}

int SysfsReader::currentFreq(int cpu) const
{
    if (!isOnline(cpu)) {
        return 0;
    }

    const QString path = QStringLiteral("%1/%2").arg(cpuPath(cpu), QLatin1String(SCALING_CUR_FREQ));
    const QString content = readFile(path);

    if (content.isEmpty()) {
        return 0;
    }

    return content.toInt();
}

QPair<int, int> SysfsReader::freqLimits(int cpu) const
{
    if (!isOnline(cpu)) {
        return qMakePair(0, 0);
    }

    const QString basePath = cpuPath(cpu);
    const QString minPath = QStringLiteral("%1/%2").arg(basePath, QLatin1String(CPUINFO_MIN_FREQ));
    const QString maxPath = QStringLiteral("%1/%2").arg(basePath, QLatin1String(CPUINFO_MAX_FREQ));

    int minFreq = readFile(minPath).toInt();
    int maxFreq = readFile(maxPath).toInt();

    return qMakePair(minFreq, maxFreq);
}

QPair<int, int> SysfsReader::scalingFreqs(int cpu) const
{
    if (!isOnline(cpu)) {
        return qMakePair(0, 0);
    }

    const QString basePath = cpuPath(cpu);
    const QString minPath = QStringLiteral("%1/%2").arg(basePath, QLatin1String(SCALING_MIN_FREQ));
    const QString maxPath = QStringLiteral("%1/%2").arg(basePath, QLatin1String(SCALING_MAX_FREQ));

    int minFreq = readFile(minPath).toInt();
    int maxFreq = readFile(maxPath).toInt();

    return qMakePair(minFreq, maxFreq);
}

QList<int> SysfsReader::availableFrequencies(int cpu) const
{
    QList<int> result;

    if (!isOnline(cpu)) {
        return result;
    }

    const QString path = QStringLiteral("%1/%2").arg(cpuPath(cpu), QLatin1String(SCALING_AVAILABLE_FREQ));
    const QStringList freqs = parseList(readFile(path));

    for (const QString &freq : freqs) {
        result.append(freq.toInt());
    }

    return result;
}

QString SysfsReader::currentGovernor(int cpu) const
{
    if (!isOnline(cpu)) {
        return QStringLiteral("OFFLINE");
    }

    const QString path = QStringLiteral("%1/%2").arg(cpuPath(cpu), QLatin1String(SCALING_GOVERNOR));
    QString governor = readFile(path);

    if (governor.isEmpty()) {
        return QStringLiteral("ERROR");
    }

    return governor;
}

QStringList SysfsReader::availableGovernors(int cpu) const
{
    const QString path = QStringLiteral("%1/%2").arg(cpuPath(cpu), QLatin1String(SCALING_AVAILABLE_GOV));
    return parseList(readFile(path));
}

QStringList SysfsReader::availableEnergyPrefs(int cpu) const
{
    const QString path = QStringLiteral("%1/%2").arg(cpuPath(cpu), QLatin1String(ENERGY_PERF_AVAIL));
    return parseList(readFile(path));
}

QString SysfsReader::currentEnergyPref(int cpu) const
{
    const QString path = QStringLiteral("%1/%2").arg(cpuPath(cpu), QLatin1String(ENERGY_PERF_PREF));
    return readFile(path);
}

bool SysfsReader::isEnergyPrefAvailable(int cpu) const
{
    const QString path = QStringLiteral("%1/%2").arg(cpuPath(cpu), QLatin1String(ENERGY_PERF_AVAIL));
    return QFile::exists(path);
}

bool SysfsReader::isOnline(int cpu) const
{
    const QList<int> online = onlineCpus();
    const QList<int> present = presentCpus();

    return present.contains(cpu) && online.contains(cpu);
}

QList<int> SysfsReader::onlineCpus() const
{
    const QString path = QStringLiteral("%1/%2").arg(QLatin1String(SYS_CPU_PATH), QLatin1String(ONLINE_FILE));
    return parseCpuList(readFile(path));
}

QList<int> SysfsReader::presentCpus() const
{
    const QString path = QStringLiteral("%1/%2").arg(QLatin1String(SYS_CPU_PATH), QLatin1String(PRESENT_FILE));
    return parseCpuList(readFile(path));
}

QList<int> SysfsReader::availableCpus() const
{
    QList<int> result;
    const QList<int> present = presentCpus();

    for (int cpu : present) {
        const QString basePath = cpuPath(cpu);
        const QString minPath = QStringLiteral("%1/%2").arg(basePath, QLatin1String(CPUINFO_MIN_FREQ));
        const QString maxPath = QStringLiteral("%1/%2").arg(basePath, QLatin1String(CPUINFO_MAX_FREQ));
        const QString govPath = QStringLiteral("%1/%2").arg(basePath, QLatin1String(SCALING_AVAILABLE_GOV));

        if (QFile::exists(minPath) && QFile::exists(maxPath) && QFile::exists(govPath)) {
            result.append(cpu);
        }
    }

    return result;
}

qint64 SysfsReader::minFrequency(int cpu) const
{
    return scalingFreqs(cpu).first;
}

qint64 SysfsReader::maxFrequency(int cpu) const
{
    return scalingFreqs(cpu).second;
}

qint64 SysfsReader::minFrequencyHardware(int cpu) const
{
    return freqLimits(cpu).first;
}

qint64 SysfsReader::maxFrequencyHardware(int cpu) const
{
    return freqLimits(cpu).second;
}
