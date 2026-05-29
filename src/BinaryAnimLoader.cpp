#include "BinaryAnimLoader.h"
#include <fstream>
#include <iostream>

bool BinaryAnimLoader::SupportsExtension(const std::wstring& extension) const
{
    return (extension == L".banm" || extension == L".BANM");
}

bool BinaryAnimLoader::Load(
    const wchar_t* filename,
    std::vector<ResMesh>& meshes,
    std::vector<ResMaterial>& materials,
    SkeletonInfo* pSkeleton,
    std::vector<AnimationClip>* pAnimations)
{
    if (!pAnimations)
        return false; // アニメーションを格納するポインタがない場合は失敗

    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile.is_open())
        return false;

    // ヘッダ読み込み
    AnimFileHeader header = {};
    inFile.read(reinterpret_cast<char*>(&header), sizeof(AnimFileHeader));

    if (header.magic != ANIM_MAGIC)
        return false; // マジックナンバーが一致しない

    AnimationClip clip;
    clip.duration = header.duration;
    clip.ticksPerSecond = header.ticksPerSecond;
    clip.name = header.clipName;

    // チャンネル（ボーンごとのアニメーション）の読み込み
    // チャンネル（ボーンごとのアニメーション）の読み込み
    clip.channels.resize(header.numChannels);
    for (uint32_t i = 0; i < header.numChannels; ++i)
    {
        AnimChannelHeader channelHeader = {};
        inFile.read(reinterpret_cast<char*>(&channelHeader), sizeof(AnimChannelHeader));
        AnimationChannel channel;
        channel.boneName = channelHeader.boneName;

        // 修正ポイント1: 変数名「matrixKeys」を使用する
        channel.matrixKeys.resize(channelHeader.numKeyFrames);
        // 修正ポイント2: 配列に直接一気に読み込む（超高速）
        if (channelHeader.numKeyFrames > 0)
        {
            inFile.read(
                reinterpret_cast<char*>(channel.matrixKeys.data()),
                sizeof(MatrixKeyFrame) * channelHeader.numKeyFrames
            );
        }
        clip.channels[i] = channel;
    }
    pAnimations->push_back(clip);
    inFile.close();
    return true;
}